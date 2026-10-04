#ifndef _EXTERNAL_MEMORY_MAPPING_HPP_
#define _EXTERNAL_MEMORY_MAPPING_HPP_

#include <assert.h>

/**
 * @brief Here should be placed description, data structures, memory parameters
 * to use external flash as mapped structure
 */
namespace external_mem_map {

constexpr uint32_t  sector_size                   = 7; // FIXME: make setable using CMake definition
constexpr uint16_t  max_sectors_amount            = 2; // FIXME: make setable using CMake definition
constexpr uint8_t   sector_rewrite_limit          = 2; // FIXME: make setable using CMake definition

static_assert(sector_rewrite_limit, "Sectors rewrite limit have to be >= 1");
static_assert(sector_size, "Sector size have to be > 0");
static_assert(max_sectors_amount, "Max sector amount have to be > 0");

typedef struct parameter_metadata_t {
  uint16_t start_sector;
  uint16_t end_sector;
  uint16_t current_sector;
  uint16_t data_offset;
  uint16_t data_size;
  uint8_t sector_rewrite_counter;
  bool data_is_read_flag;
} parameter_metadata_t;

} // namespace externalMemMap

#endif // _EXTERNAL_MEMORY_MAPPING_HPP_
