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

Storage::rw_status Storage::write_data_structure(void) {
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
  // XXX: maybe crc8 should be written by separate command to solve problem below
  // XXX: there can be used packed structures
  flash_data_buffer.crc8 = compute_crc8(
    (uint8_t *) &flash_data_buffer, sizeof(flash_data_buffer) - 1 // FIXME: size can be different due to alignment!!
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
      metadata_info.sector_num = 0;
    } else {
      // available sectors aren't reached, use next next one
      metadata_info.sector_num++;
    }
  } else { // rewrite is available, reuse sector
    metadata_buffer.current_rewrite_counter++;
  }
  inner_flags.is_there_metadata_free_space = 1;
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
  return rw_status::OK; // metadata was updated
}

Storage::rw_status Storage::read_last_metadata(void) { // TODO: update offsets, flags etc.
  metadata_t metadata_buffer_prev;
  size_t ff_byte_counter;
  uint8_t *ptr, *ptr_copy;

  if (inner_flags.data_is_read) {
    // metadata already has been read
    return rw_status::OK;
  }

  if (read_bytes == nullptr) {
    return rw_status::NULLPTR_ERROR;
  }

  while (metadata_info.sector_num < sectors_amount_for_metadata) {
    ff_byte_counter = 0; // clear current 0xFF bytes counter
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
    // check and save last data
    ptr_copy = (uint8_t *) &metadata_buffer_prev;
    for (
      ptr = (uint8_t *) &metadata_buffer;
      ptr < (uint8_t *) (&metadata_buffer.crc8 + 1);
      ptr++
    ) {
      if (*ptr == 0xFF) {
        ff_byte_counter++;
      }
      *(ptr_copy++) = *ptr; // copy metadata
    }
    if (ff_byte_counter == sizeof(metadata_t)) {
      // found clean memory cells, relevant data is placed into `metadata_buffer_prev`
      break;
    }
    // update metadata info
    metadata_info.offset += sizeof(metadata_t);
    if (metadata_info.offset >= 4095) {
      if (++metadata_info.sector_num >= sectors_amount_for_metadata) {
        break;
      }
      // work with next sector, clear offset
      metadata_info.offset = 0;
    }
  }
  // check clean memory case
  if (!metadata_info.offset && !metadata_info.sector_num) {
    return rw_status::NO_METADATA; // XXX: in that case there should be used default values for data saving
  }
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
    // incorrect data, use default values to keep data
    rw_status s{clean_memory()};
    if (s != rw_status::OK) {
      return s;
    }
    return rw_status::NO_METADATA;
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
  // no useful metadata, erase available memory used for
  if (!erase_all_sectors()) {
    return rw_status::ERASE_METADATA_FAILED;
  }
  rw_status s{clean_memory()};
  if (s != rw_status::OK) {
    return s;
  }
  return rw_status::NO_METADATA;
}

Storage::rw_status Storage::read_last_data(void) { /* TODO: update offsets, flags etc.*/ }

Storage::rw_status Storage::clean_memory(void) {
  if (!erase_all_sectors()) {
    return rw_status::ERASE_METADATA_FAILED;
  }
  // set and save default values
  metadata_info.sector_num = 0;
  metadata_info.offset = 0;
  metadata_buffer.data_sector_num = sectors_amount_for_metadata;
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
