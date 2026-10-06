#ifndef _STORAGE_HPP_
#define _STORAGE_HPP_

#include <functional>
#include <cstdint>
#if (__cplusplus < 202002L) || (_MSVC_LANG < 202002L)
#include <type_traits>
#endif
#include "external_memory_mapping.hpp"

namespace flash_storage {

using namespace external_mem_map;

struct __attribute__((__packed__)) base_data_t {
  uint8_t crc8;
  uint16_t rewrite_counter;
};

class Storage {
public:
  enum class rw_status : int8_t {
    BASE_STRUCT_ERROR     = -10,
    CRC8_ERROR            = -9,
    ERASE_DATA_FAILED     = -8,
    NO_DATA               = -7,
    READ_FAILED           = -6,
    DATA_OFFSET_ERROR     = -5,
    WRITE_DATA_FAILED     = -4,
    DATA_SIZE_ERROR       = -3,
    SECTORS_LIMITS_ERROR  = -2,
    NULLPTR_ERROR         = -1,
    OK                    = 0,
    END
  };

  // setters
  bool set_read_bytes(
    // sector number, offset, buffer pointer, buffer size
    std::function<bool(uint32_t, uint32_t, uint8_t*, size_t)> f
  );
  bool set_write_bytes(
    // sector number, offset, buffer pointer, buffer size
    std::function<bool(uint32_t, uint32_t, uint8_t*, size_t)> f
  );
  bool set_erase_sectors(
    // sector number, sectors amount to erase
    std::function<bool(uint32_t, uint32_t)> f
  );
  bool set_erase_all(std::function<bool(void)> f);
  bool set_compute_crc8(
    // buffer pointer, buffer size
    std::function<uint8_t(uint8_t *, uint8_t)> f
  );
  // r/w operations
  Storage::rw_status read_data_structure(parameter_metadata_t *p_mdata, base_data_t *p_data);
  Storage::rw_status write_data_structure(parameter_metadata_t *p_mdata, base_data_t *p_data);

private:
  // Methods
  std::function<bool(uint32_t, uint32_t, uint8_t*, size_t)> read_bytes {
    // sector number, offset, buffer pointer, buffer size
    [] (uint32_t, uint32_t, uint8_t*, size_t) { return false; }
  };
  std::function<bool(uint32_t, uint32_t, uint8_t*, size_t)> write_bytes {
    // sector number, offset, buffer pointer, buffer size
    [] (uint32_t, uint32_t, uint8_t*, size_t) { return false; }
  };
  std::function<bool(uint32_t, uint32_t)> erase_sectors {
    // sector number, sectors amount to erase
    [] (uint32_t, uint32_t) { return false; }
  };
  std::function<bool(void)> erase_all_sectors {
    [] { return false; }
  };
  std::function<uint8_t(uint8_t *, uint8_t)> compute_crc8 {
    [] (uint8_t *, uint8_t) { return 0; }
  };
  rw_status clean_memory(void);
};

namespace func_types {
  using read_write_t = bool(*)(uint32_t, uint32_t, uint8_t*, size_t);
  using erase_sectors_t = bool(*)(uint32_t, uint32_t);
  using erase_all_sectors_t = bool(*)(void);
  using compute_crc8_t = uint8_t(*)(uint8_t *, uint8_t);
}

#if (__cplusplus >= 202002L) || (_MSVC_LANG >= 202002L)
template<func_types::read_write_t read_bytes,
  func_types::read_write_t write_bytes,
  func_types::erase_sectors_t erase_sectors,
  func_types::erase_all_sectors_t erase_all_sectors,
  func_types::compute_crc8_t compute_crc8>
requires (read_bytes != nullptr) &&
  (write_bytes != nullptr) &&
  (erase_sectors != nullptr) &&
  (erase_all_sectors != nullptr) &&
  (compute_crc8 != nullptr)
#else
template<func_types::read_write_t read_bytes,
  func_types::read_write_t write_bytes,
  func_types::erase_sectors_t erase_sectors,
  func_types::erase_all_sectors_t erase_all_sectors,
  func_types::compute_crc8_t compute_crc8,
  std::enable_if_t<(
    (read_bytes != nullptr) &&
    (write_bytes != nullptr) &&
    (erase_sectors != nullptr) &&
    (erase_all_sectors != nullptr) &&
    (compute_crc8 != nullptr)
  ), int> = 0
>
#endif
struct StructStorage {
  // r/w operations
  Storage::rw_status read_data_structure(
    parameter_metadata_t *p_mdata, base_data_t *p_data
  ) {
    if (p_mdata == nullptr || p_data == nullptr) {
      return Storage::rw_status::NULLPTR_ERROR;
    }
    if (p_mdata->end_sector < p_mdata->start_sector) {
      return Storage::rw_status::SECTORS_LIMITS_ERROR;
    }
    if (p_mdata->data_offset > sector_size) {
      return Storage::rw_status::DATA_OFFSET_ERROR;
    }
    if (!p_mdata->data_size) {
      return Storage::rw_status::DATA_SIZE_ERROR;
    }
    if (p_mdata->data_size <= sizeof(base_data_t)) {
      return Storage::rw_status::BASE_STRUCT_ERROR;
    }

    if (!p_mdata->data_is_read_flag) {
      bool read_status{false};
      uint16_t prev_sector{p_mdata->start_sector};
      uint16_t prev_offset{0};
      for (uint16_t sector{p_mdata->start_sector}; sector <= p_mdata->end_sector; sector++) {
        for (size_t offset{0}; offset <= (sector_size - p_mdata->data_size); offset += p_mdata->data_size) {
          read_status = read_bytes(
            sector, offset,
            reinterpret_cast<uint8_t *>(p_data), p_mdata->data_size
          );
          if ((compute_crc8(
              reinterpret_cast<uint8_t *>(p_data) + sizeof(p_data->crc8), p_mdata->data_size - 1
            ) == p_data->crc8) && read_status
          ) {
            // data is correct
            prev_offset = offset;
            p_mdata->current_sector = sector;
            p_mdata->data_offset = offset;
            p_mdata->sector_rewrite_counter = p_data->rewrite_counter;
            continue;
          }
          if (!read_status) {
            return Storage::rw_status::READ_FAILED;
          }
          // incorrect data was read
          if ((sector == p_mdata->start_sector) && !offset) {
            // beginning of the memory
            // clear memory, there is no useful data
            if (!erase_sectors(p_mdata->start_sector, p_mdata->end_sector - p_mdata->start_sector + 1)) {
              return Storage::rw_status::ERASE_DATA_FAILED;
            }
            // set default values
            p_mdata->current_sector = p_mdata->start_sector;
            p_mdata->data_offset = 0;
            p_mdata->sector_rewrite_counter = 0;
            // return to show that there should be written default values in p_data
            return Storage::rw_status::NO_DATA;
          }
          // there is correct data, read it again
          read_status = read_bytes(
            prev_sector, prev_offset,
            reinterpret_cast<uint8_t *>(p_data), p_mdata->data_size
          );
          if (read_status) {
            p_mdata->data_is_read_flag = true;
            return Storage::rw_status::OK;
          }
          return Storage::rw_status::READ_FAILED;
        }
        prev_sector = sector;
      }
      return Storage::rw_status::READ_FAILED;
    }
    return Storage::rw_status::OK;
  }

  Storage::rw_status write_data_structure(
    parameter_metadata_t *p_mdata, base_data_t *p_data
  ) {
    if (p_data == nullptr || p_mdata == nullptr) {
      return Storage::rw_status::NULLPTR_ERROR;
    }
    if (p_mdata->end_sector < p_mdata->start_sector) {
      return Storage::rw_status::SECTORS_LIMITS_ERROR;
    }
    if (p_mdata->data_offset > sector_size) {
      return Storage::rw_status::DATA_OFFSET_ERROR;
    }
    if (!p_mdata->data_size) {
      return Storage::rw_status::DATA_SIZE_ERROR;
    }
    if (p_mdata->data_size <= sizeof(base_data_t)) {
      return Storage::rw_status::BASE_STRUCT_ERROR;
    }

    uint16_t offset_backup{p_mdata->data_offset};
    uint16_t rewrite_counter_backup{p_mdata->sector_rewrite_counter};
    uint16_t sector_index_backup{p_mdata->current_sector};
    int32_t free_bytes;
    if (p_mdata->data_is_read_flag) {
      // data has been already written
      free_bytes = (int32_t) external_mem_map::sector_size - p_mdata->data_offset - p_mdata->data_size;
    } else {
      free_bytes = (int32_t) external_mem_map::sector_size - p_mdata->data_offset;
    }
    // check free space before writing
    if (free_bytes < p_mdata->data_size) {
      // not enought free space
      if (p_mdata->sector_rewrite_counter == external_mem_map::sector_rewrite_limit) {
        // go to the next sector or start from the beginning
        if (p_mdata->current_sector == p_mdata->end_sector) {
          // start from the beginning
          // clear available memory
          if (!erase_sectors(p_mdata->start_sector, p_mdata->end_sector)) {
            return Storage::rw_status::ERASE_DATA_FAILED;
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
          return Storage::rw_status::ERASE_DATA_FAILED;
        }
        p_mdata->data_offset = 0;
        p_mdata->sector_rewrite_counter++;
      }
    } else if (p_mdata->data_is_read_flag) {
      // continue to write to current sector 
      p_mdata->data_offset += p_mdata->data_size;
    }
    // update sector rewrite counter
    p_data->rewrite_counter = p_mdata->sector_rewrite_counter;
    // update crc8
    p_data->crc8 = compute_crc8(
      reinterpret_cast<uint8_t *>(p_data) + sizeof(p_data->crc8), p_mdata->data_size - 1
    );
    // write data
    if (
      !write_bytes(
        p_mdata->current_sector,
        p_mdata->data_offset,
        reinterpret_cast<uint8_t *>(p_data),
        p_mdata->data_size
      )
    ) {
      // restore origin values
      p_mdata->data_offset = offset_backup;
      p_mdata->sector_rewrite_counter = rewrite_counter_backup;
      p_mdata->current_sector = sector_index_backup;
      return Storage::rw_status::WRITE_DATA_FAILED;
    }
    p_mdata->data_is_read_flag = true;
    // TODO: read data and compare for checking write operation
    return Storage::rw_status::OK;
  }
};

} // namespace flash_storage

#endif // _STORAGE_HPP_
