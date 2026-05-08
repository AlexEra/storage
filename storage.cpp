#include "storage.hpp"

using namespace flash_storage;

bool Storage::set_read_bytes(
  std::function<bool(uint8_t, uint32_t, uint8_t*, size_t)> f
) {
  if (f == nullptr) {
    return false;
  }
  read_bytes = f;
  return true;
}

bool Storage::set_write_bytes(
  std::function<bool(uint32_t, uint32_t, uint8_t*, size_t)> f
) {
  if (f == nullptr) {
    return false;
  }
  write_bytes = f;
  return true;
}

bool Storage::set_erase_sectors(
  std::function<bool(uint32_t, uint32_t)> f
) {
  if (f == nullptr) {
    return false;
  }
  erase_sectors = f;
  return true;
}

bool Storage::set_erase_all(std::function<bool(void)> f) {
  if (f == nullptr) {
    return false;
  }
  erase_all_sectors = f;
  return true;
}

bool Storage::set_compute_crc8(
  // buffer pointer, buffer size
  std::function<uint8_t(uint8_t *, uint8_t)> f
) {
  if (f == nullptr) {
    return false;
  }
  compute_crc8 = f;
  return true;
}

Storage::rw_status Storage::read_data_structure(
  parameter_metadata_t *p_mdata,
  uint8_t *p_data
) {
  if (
    read_bytes == nullptr || p_mdata == nullptr ||
    p_data == nullptr || erase_sectors == nullptr ||
    compute_crc8 == nullptr
  ) {
    return rw_status::NULLPTR_ERROR;
  }
  if (p_mdata->end_sector < p_mdata->start_sector) {
    return rw_status::SECTORS_LIMITS_ERROR;
  }
  if (p_mdata->data_offset > sector_size) {
    return rw_status::DATA_OFFSET_ERROR;
  }
  if (!p_mdata->data_size) {
    return rw_status::DATA_SIZE_ERROR;
  }

  rw_status rw_status;
  if (!data_is_read_flag) {
    size_t offset{0};
    bool read_status{false};
    uint16_t prev_sector{p_mdata->start_sector};
    uint16_t prev_offset{0};
    for (uint16_t sector{p_mdata->start_sector}; sector < p_mdata->end_sector; sector++) {
      for (; offset < (sector_size - p_mdata->data_size); offset += p_mdata->data_size) {
        read_status = read_bytes(sector, offset, p_data, p_mdata->data_size);
        if (!compute_crc8(p_data, p_mdata->data_size) || !read_status) {
          // data is correct
          prev_offset = offset;
          p_mdata->current_sector = sector;
          p_mdata->data_offset = offset;
          // XXX: rewrite counter must be the pre-last byte
          p_mdata->sector_rewrite_counter = *((uint16_t *)(p_data + p_mdata->data_size - 3));
          continue;
        }
        // incorrect data was read
        if ((sector == p_mdata->start_sector) && !offset) {
          // beginning of the memory
          // clear memory, there is no useful data
          if (!erase_sectors(p_mdata->start_sector, p_mdata->end_sector - p_mdata->start_sector + 1)) {
            return rw_status::ERASE_DATA_FAILED;
          }
          // set default values
          p_mdata->current_sector = p_mdata->start_sector;
          p_mdata->data_offset = 0;
          p_mdata->sector_rewrite_counter = 0;
          // return to show that there should be written default values in p_data
          return rw_status::NO_DATA;
        }
        // there is correct data, read it again
        read_status = read_bytes(prev_sector, prev_offset, p_data, p_mdata->data_size);
        if (read_status) {
          data_is_read_flag = true;
          return rw_status::OK;
        }
        return rw_status::READ_FAILED;
      }
      prev_sector = sector;
    }
  }
  return rw_status::OK;
}

Storage::rw_status Storage::write_data_structure(
  parameter_metadata_t *p_mdata,
  uint8_t *p_data
) {
  if (
    p_data == nullptr || p_mdata == nullptr ||
    write_bytes == nullptr || erase_sectors == nullptr ||
    compute_crc8 == nullptr
  ) {
    return rw_status::NULLPTR_ERROR;
  }
  if (p_mdata->end_sector < p_mdata->start_sector) {
    return rw_status::SECTORS_LIMITS_ERROR;
  }
  if (p_mdata->data_offset > sector_size) {
    return rw_status::DATA_OFFSET_ERROR;
  }
  if (!p_mdata->data_size) {
    return rw_status::DATA_SIZE_ERROR;
  }

  uint16_t offest_backup = p_mdata->data_offset;
  uint16_t rewrite_counter_backup = p_mdata->sector_rewrite_counter;
  uint16_t sector_index_backup = p_mdata->current_sector;
  // update crc8
  *(p_data + p_mdata->data_size - 1) = compute_crc8( // XXX: last byte should be CRC8
    p_data, p_mdata->data_size - 1
  );
  // check free space before writing
  if (
    (external_mem_map::sector_size - p_mdata->data_offset - p_mdata->data_size) < p_mdata->data_size
  ) {
    // not enought free space
    if (p_mdata->sector_rewrite_counter == external_mem_map::sector_rewrite_limit) {
      // go to the next sector or start from the beginning
      if (p_mdata->current_sector == p_mdata->end_sector) {
        // start from the beginning
        // clear available memory
        if (!erase_sectors(p_mdata->start_sector, p_mdata->end_sector)) {
          return rw_status::ERASE_DATA_FAILED;
        }
        p_mdata->current_sector = p_mdata->start_sector;
      } else {
        // go to the next sector, it should be already cleaned
        p_mdata->current_sector++;
      }
      p_mdata->data_offset = 0;
      p_mdata->sector_rewrite_counter = 0;
    } else {
      // reuse sector
      if (!erase_sectors(p_mdata->current_sector, p_mdata->current_sector)) {
        return rw_status::ERASE_DATA_FAILED;
      }
      p_mdata->data_offset = 0;
      p_mdata->sector_rewrite_counter++;
    }
  } else {
    // continue to write to current sector 
    p_mdata->data_offset += p_mdata->data_size;
  }
  // update sector rewrite counter
  *(p_data + p_mdata->data_size - 2) = p_mdata->sector_rewrite_counter;

  // write data
  if (
    !write_bytes(
      p_mdata->current_sector,
      p_mdata->data_offset,
      p_data,
      p_mdata->data_size
    )
  ) {
    // restore origin values
    p_mdata->data_offset = offest_backup;
    p_mdata->sector_rewrite_counter = rewrite_counter_backup;
    p_mdata->current_sector = sector_index_backup;
    return rw_status::WRITE_DATA_FAILED;
  }
  // TODO: read data and compare for checking write operation
  return rw_status::OK;
}

Storage::rw_status Storage::clean_memory(void) {
  if (erase_all_sectors == nullptr) {
    return rw_status::NULLPTR_ERROR;
  }
  if (!erase_all_sectors()) {
    return rw_status::ERASE_DATA_FAILED;
  }
  return rw_status::OK;
}
