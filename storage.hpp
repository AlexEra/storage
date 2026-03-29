#ifndef _STORAGE_HPP_
#define _STORAGE_HPP_

#include <functional>
#include <cstdint>
#include <string.h>

#include "main_data_structure.h"
#include "prev_data_structure.h"


namespace flash_storage {

constexpr uint32_t  sector_size                   = 4096;
constexpr uint8_t   sectors_amount_for_metadata   = 2;
constexpr uint8_t   sector_rewrite_limit          = 5;

class Storage {
public:
  enum class rw_status {
    CRC8_ERROR            = -9,
    ERASE_DATA_FAILED     = -8,
    ERASE_METADATA_FAILED = -7,
    READ_FAILED           = -6,
    WRITE_METADATA_FAILED = -5,
    WRITE_DATA_FAILED     = -4,
    NO_DATA               = -3,
    NO_METADATA           = -2,
    NULLPTR_ERROR         = -1,
    OK                    = 0,
    END
  };

  // setters
  bool set_read_bytes(
    // sector number, buffer pointer, buffer size
    std::function<bool(uint8_t, uint32_t, uint8_t*, size_t)> f
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
  bool set_max_sectors(uint16_t max_sectors);
  bool set_compute_crc8(
    // buffer pointer, buffer size
    std::function<uint8_t(uint8_t *, uint8_t)> f
  );
  // r/w operations
  Storage::rw_status read_data_structure(void);
  Storage::rw_status write_data_structure(void);

  // Attributes
  main_flash_data_t flash_data_buffer;

private:
  typedef struct metadata_info_t {
    uint32_t offset;
    uint8_t sector_num;
  } metadata_info_t;

  typedef struct metadata_t {
    uint32_t data_sector_num;
    uint16_t current_rewrite_counter;
    uint8_t crc8;
  } metadata_t;

  typedef struct flags_t {
    uint8_t data_is_read: 1;
    uint8_t is_there_data_free_space: 1;
    uint8_t is_there_metadata_free_space: 1;
    uint8_t reserved: 5;
  } flags_t;

  // Methods
  std::function<bool(uint8_t, uint32_t, uint8_t*, size_t)> read_bytes {
    // sector number, offset, buffer pointer, buffer size
    [] (uint32_t, uint8_t*, size_t) { return false; }
  };
  std::function<bool(uint8_t, uint32_t, uint8_t*, size_t)> write_bytes {
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
  bool prepare_data_free_space(void);
  bool prepare_metadata_free_space(void);
  rw_status update_metadata(void);
  rw_status read_last_metadata(void); // TODO: update offsets, flags etc.
  rw_status read_last_data(void); // TODO: update offsets, flags etc.

  // Attributes
  metadata_t metadata_buffer{
    .data_sector_num{0},
    .current_rewrite_counter{0},
    .crc8 = 0
  };
  metadata_info_t metadata_info{
    .offset = 0,
    .sector_num = 0
  };
  uint32_t data_struct_offset{0};
  flags_t inner_flags{0};
  uint16_t _max_sectors{0};
};

} // namespace flash_storage

#endif // _STORAGE_HPP_
