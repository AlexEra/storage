#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _MAIN_DATA_STRUCTURE_H_
#define _MAIN_DATA_STRUCTURE_H_

// Structure to keep main data for flash

#pragma pack(push, 1)
typedef struct main_flash_data_t {
  uint32_t version; // do not delete this
  uint8_t rewrite_counter; // do not delete this

  // data section begin

  // data section end

  uint8_t crc8; // do not delete this
} main_flash_data_t;
#pragma pack(pop)

#endif // _MAIN_DATA_STRUCTURE_H_

#ifdef __cplusplus
}
#endif
