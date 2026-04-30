#ifndef _EXTERNAL_MEMORY_MAPPING_HPP_
#define _EXTERNAL_MEMORY_MAPPING_HPP_

#include <assert.h>

/**
 * @brief Here should be placed description, data structures, memory parameters
 * to use external flash as mapped structure
 */
namespace external_mem_map {

constexpr uint32_t  sector_size                   = 10; // TODO: set to 4096 for release
constexpr uint16_t  max_sectors_amount            = 2;  // TODO: set to 8192 (?) for 32 MB flash
constexpr uint8_t   sector_rewrite_limit          = 5;  // variable 

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
} parameter_metadata_t;

} // namespace externalMemMap

#endif // _EXTERNAL_MEMORY_MAPPING_HPP_
