#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _PREV_DATA_STRUCTURE_H_
#define _PREV_DATA_STRUCTURE_H_

// Version 0 is reserved for structure without any payload data

typedef struct prev_flash_data_t {
  uint32_t version; // do not delete this
  uint8_t rewrite_counter; // do not delete this

  // data section begin
  
  // data section end

  uint8_t crc8; // do not delete this
} prev_flash_data_t;

#endif // _PREV_DATA_STRUCTURE_H_

#ifdef __cplusplus
}
#endif
