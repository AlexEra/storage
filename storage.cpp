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

bool Storage::set_max_sectors(uint16_t max_sectors) {
  if (!max_sectors) {
    _max_sectors = max_sectors;
    return true;
  }
  return false;
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

Storage::rw_status Storage::read_data_structure(void) {
  if (read_bytes == nullptr) {
    return rw_status::NULLPTR_ERROR;
  }

  // check metadata update
  rw_status rw_status{update_metadata()};

  if (rw_status != rw_status::OK) {
    return rw_status;
  }

  // metadata is known
  rw_status = read_last_data();
  if (rw_status != rw_status::OK) {
    // main data structure wasn't found
    return rw_status;
  }
  // data was read successfuly
  inner_flags.data_is_read = 1;
  return rw_status::OK;
}

Storage::rw_status Storage::write_data_structure(void)  {
  if (
    write_bytes == nullptr ||
    erase_sectors == nullptr ||
    !_max_sectors
  ) {
    return rw_status::NULLPTR_ERROR;
  }

  // check metadata update
  rw_status rw_status{update_metadata()};

  if (rw_status != rw_status::OK) {
    return rw_status;
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
  flash_data_buffer.rewrite_counter++;

  // check sector changes
  if (metadata_buffer.data_sector_num != last_sectors_amount) {
    // sector was updated, rewrite metadata
    if (!inner_flags.is_there_metadata_free_space) {
      // not enough free space, erase sector/-s
      if (!prepare_metadata_free_space) {
        return rw_status::ERASE_METADATA_FAILED;
      }
    }
  }

  // update metadata
  metadata_buffer.crc8 = compute_crc8(
    (uint8_t *) &metadata_buffer, sizeof(metadata_t) - 2
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

  metadata_buffer.current_rewrite_counter++;
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
          metadata_buffer.data_sector_num - sectors_amount_for_metadata
        )
      ) {
        // erasing failed
        return false;
      }
      metadata_buffer.data_sector_num = sectors_amount_for_metadata;
    } else {
      // reuse sector
      if (!erase_sectors(metadata_buffer.data_sector_num, 1)) {
        // erasing failed
        return false;
      }
    }
  }
  return true;
}

bool Storage::prepare_metadata_free_space(void) {
  if (metadata_buffer.current_rewrite_counter == sector_rewrite_limit) {
    // rewrite limit was reached, prepare memory for new writings
    metadata_buffer.current_rewrite_counter = 0;
    metadata_info.offset = 0;
    if (metadata_info.sector_num == (sectors_amount_for_metadata - 1)) {
      // end of the available flash for metadata was reached
      // erase available memory
      if (!erase_sectors(0, sectors_amount_for_metadata)) {
        // erasing failed
        return false;
      }
    } else {
      // reuse sector
      if (!erase_sectors(metadata_info.sector_num, 1)) {
        // erasing failed
        return false;
      }
    }
  }
  return true;
}

Storage::rw_status Storage::update_metadata(void) {
  if (!inner_flags.data_is_read) {
    // first launch, metadata is clear, update it
    rw_status rw_status = read_last_metadata();
    if (rw_status != rw_status::OK) {
      // metadata wasn't found
      metadata_buffer.data_sector_num = sectors_amount_for_metadata;
      metadata_buffer.current_rewrite_counter = 0;
      metadata_info.offset = 0;
      metadata_info.sector_num = 0;
      inner_flags.is_there_metadata_free_space = 1;
      metadata_buffer.crc8 = compute_crc8(
        (uint8_t *) &metadata_buffer, sizeof(metadata_t) - 2
      );
      inner_flags.is_there_data_free_space = 1;
      data_struct_offset = 0;
      return rw_status;
    }
  }
}

rw_status read_last_metadata(void) { // TODO: update offsets, flags etc.
  if (inner_flags.data_is_read) {
    // metadata already has been read
    return rw_status::OK;
  }

  if (read_bytes == nullptr) {
    return rw_status::NULLPTR_ERROR;
  }

  metadata_t metadata_buffer_prev;
  size_t ff_byte_counter{0};
  while (metadata_info.sector_num < sectors_amount_for_metadata) {
    // read data
    if (
      !read_bytes(
        metadata_info.sector_num,
        metadata_info.offset,
        (uint8_t *) &metadata_buffer,
        sizeof(metadata_t)
      )
    ) {
      return rw_status::READ_FAILED;
    }
    // save last data
    memcpy(&metadata_buffer_prev, &metadata_buffer, sizeof(metadata_t));
    // check data
    for (
      uint8_t *ptr = (uint8_t *) &metadata_buffer;
      ptr < (uint8_t *) (&metadata_buffer.crc8 + 1);
      ptr++
    ) {
      if (*ptr == 0xFF) { // FIXME: нас интересуют пустые ячейки
        ff_byte_counter++;
      }
    }
    if (ff_byte_counter == sizeof(metadata_t)) {
      // found clean memory cells
      break;
    }
    // update metadata info
    metadata_info.offset += sizeof(metadata_t);
    if (metadata_info.offset >= 4095) {
      if (++metadata_info.sector_num >= sectors_amount_for_metadata) {
        break;
      }
      metadata_info.offset = 0;
    }
  }
  // FIXME: если были считаны первые ячейки памяти, то в metadata_buffer_prev - мусор
  // check for reaching the end of available metadata memory
  if (
    (metadata_info.sector_num >= sectors_amount_for_metadata) && // last available sector
    (metadata_info.offset >= 4095)
  ) {
    // check last data
    if (
      compute_crc8(
        (uint8_t *) &metadata_buffer_prev, sizeof(metadata_t) - 2
      ) == metadata_buffer_prev.crc8
    ) {
      inner_flags.data_is_read = 1;
      inner_flags.is_there_metadata_free_space = 0;
      return rw_status::OK;
    }
    // no useful metadata, erase available memory used for
    // FIXME: надо очищать сектора под данные
    // FIXME проще очистить чип
    if (!erase_sectors(0, sectors_amount_for_metadata)) {
      return rw_status::ERASE_METADATA_FAILED;
    }
    // set and save default values
    metadata_info.sector_num = 0;
    metadata_info.offset = 0;
    metadata_buffer.data_sector_num = 0;
    metadata_buffer.current_rewrite_counter = 0;
    metadata_buffer.crc8 = compute_crc8(
      (uint8_t *) &metadata_buffer, sizeof(metadata_t) - 2
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
    inner_flags.data_is_read = 1;
    inner_flags.is_there_metadata_free_space = 1;
    return rw_status::OK;
  }
  // general case
  // check last data
  if (
    compute_crc8(
      (uint8_t *) &metadata_buffer_prev, sizeof(metadata_t) - 2
    ) == metadata_buffer_prev.crc8
  ) {
    inner_flags.data_is_read = 1;
    inner_flags.is_there_metadata_free_space = 1;
    return rw_status::OK;
  }
  // TODO: надо что-то делать, если считаны некорректные данные
  // TODO: проще всего очистить чип
}
rw_status read_last_data(void) { /* TODO: update offsets, flags etc.*/ }
