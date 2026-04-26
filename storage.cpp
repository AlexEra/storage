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

// TODO: maybe data should be written from the end of available memory, or should be read from the end of it

Storage::rw_status Storage::read_data_structure(
  parameter_metadata_t *p_mdata,
  uint8_t *p_data
) {
  if (read_bytes == nullptr || p_mdata == nullptr || p_data == nullptr) {
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
  if (!inner_flags.data_is_read) {
    // TODO: read last data
    size_t offset{0};
    bool read_status{false};
    uint8_t ff_bytes_counter;
    for (uint16_t sector{p_mdata->start_sector}; sector < p_mdata->end_sector; sector++) {
      for (; offset < (sector_size - p_mdata->data_size); offset += p_mdata->data_size) {
        ff_bytes_counter = 0;
        read_status = read_bytes(sector, offset, p_data, p_mdata->data_size);
        if (read_status) {
          // TODO: check ff bytes
          for (uint8_t *ptr{p_data}; ptr < (p_data + p_mdata->data_size); ptr++) {
            if (0xFF == *ptr) {
              ff_bytes_counter++;
            } else {
              break;
            }
          }
          if (ff_bytes_counter == p_mdata->data_size) {
            // clean memory, check prev data
            // FIXME: last data can be placed at the previous sector
            // TODO: check that this is not the beginning
            if (compute_crc8(p_data, p_mdata->data_size)) { // FIXME: use std::span?
              // there are no data
              // TODO: check, that correct data was read to return it
              if (erase_sectors(p_mdata->start_sector, p_mdata->end_sector - p_mdata->start_sector + 1)) {
                // return to show that there should be written default values in p_data
                return rw_status::NO_DATA;
              }
              return rw_status::ERASE_DATA_FAILED;
            }
          }
        }
      }
    }
    // TODO: check crc8
    inner_flags.data_is_read = 1;
  } else {
    // TODO: read data 
  }

  if (rw_status != rw_status::OK) {
    // main data structure wasn't found
    return rw_status;
  }
  // data was read successfuly
  return rw_status::OK;
}

Storage::rw_status Storage::write_data_structure(
  parameter_metadata_t *p_mdata,
  uint8_t *p_data
) {
  if (
    write_bytes == nullptr ||
    erase_sectors == nullptr ||
    !
  ) {
    return rw_status::NULLPTR_ERROR;
  }

  // check free space
  uint32_t last_sectors_amount = metadata_buffer.data_sector_num;
  if (!inner_flags.is_there_data_free_space) {
    // not enough free space, erase sector/-s
    if (!prepare_data_free_space()) {
      return rw_status::ERASE_DATA_FAILED;
    }
  }

  // update crc8
  // XXX: maybe crc8 should be written by separate command to solve problem below
  flash_data_buffer.crc8 = compute_crc8(
    (uint8_t *) &flash_data_buffer, sizeof(flash_data_buffer) - 1
  );
  // write data
  if (
    !write_bytes(
      metadata_buffer.data_sector_num * sector_size,
      data_struct_offset,
      (uint8_t *) &flash_data_buffer,
      sizeof(flash_data_buffer)
    )
  ) {
    return rw_status::WRITE_DATA_FAILED;
  }
  // write success, update offset
  data_struct_offset += sizeof(flash_data_buffer);
  if (data_struct_offset >= sector_size) {
    // we've reached the end of the sector memory
    inner_flags.is_there_data_free_space = 0;
  }

  // check sector changes
  if (metadata_buffer.data_sector_num != last_sectors_amount) {
    // sector was updated, rewrite metadata
    if (!inner_flags.is_there_metadata_free_space) {
      // not enough free space, erase sector/-s
      if (!prepare_metadata_free_space()) {
        return rw_status::ERASE_METADATA_FAILED;
      }
    }
  }

  // update metadata
  metadata_buffer.crc8 = compute_crc8(
    (uint8_t *) &metadata_buffer, sizeof(metadata_t) - 1
  );
  if (
    !write_bytes(
      metadata_info.sector_num,
      metadata_info.offset,
      (uint8_t *) &metadata_buffer,
      sizeof(metadata_buffer)
    )
  ) {
    return rw_status::WRITE_METADATA_FAILED;
  }
  // update offset
  metadata_info.offset += sizeof(metadata_info_t);
  if (metadata_info.offset >= sector_size) {
    // we've reached the end of the sector memory
    inner_flags.is_there_metadata_free_space = 0;
  }
  return rw_status::OK;
}

bool Storage::prepare_data_free_space(void) {
  if (flash_data_buffer.rewrite_counter == sector_rewrite_limit) {
    // rewrite limit was reached, prepare memory for new writings
    flash_data_buffer.rewrite_counter = 0;
    data_struct_offset = 0;
    if (metadata_buffer.data_sector_num == (_max_sectors - 1)) {
      // end of the flash was reached, erase available memory
      if (
        !erase_sectors(
          sectors_amount_for_metadata,
          metadata_buffer.data_sector_num - sectors_amount_for_metadata + 1
        )
      ) {
        // erasing failed
        return false;
      }
      metadata_buffer.data_sector_num = sectors_amount_for_metadata;
    } else {
      // available sectors aren't reached, use next next one
      metadata_buffer.data_sector_num++;
    }
  } else { // rewrite is available, reuse sector
    flash_data_buffer.rewrite_counter++;
  }
  inner_flags.is_there_data_free_space = 1; // free space is available
  return true;
}

Storage::rw_status Storage::clean_memory(void) {
  if (!erase_all_sectors()) {
    return rw_status::ERASE_METADATA_FAILED;
  }
  // TODO: should it be finished?
  inner_flags.is_there_metadata_free_space = 1;
  return rw_status::OK;
}
