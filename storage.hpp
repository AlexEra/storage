#ifndef _STORAGE_HPP_
#define _STORAGE_HPP_

#include <functional>
#include <cstdint>
#include <string.h>

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

} // namespace flash_storage

#endif // _STORAGE_HPP_
