#ifndef _EXTERNAL_MEMORY_MAPPING_HPP_
#define _EXTERNAL_MEMORY_MAPPING_HPP_

#include <assert.h>

#ifndef STORAGE_SECTOR_SIZE
#define STORAGE_SECTOR_SIZE 4096 // default flash sector size
#endif /* STORAGE_SECTOR_SIZE */

#ifndef STORAGE_MAX_SECTORS_AMOUNT
#define STORAGE_MAX_SECTORS_AMOUNT 256 // amount for 1MB flash
#endif /* STORAGE_MAX_SECTORS_AMOUNT */

#ifndef STORAGE_SECTOR_REWRITE_LIMIT
#define STORAGE_SECTOR_REWRITE_LIMIT 2 // default rewrite
#endif /* STORAGE_SECTOR_REWRITE_LIMIT */

/**
 * @brief Here should be placed description, data structures, memory parameters
 * to use external flash as mapped structure
 */
namespace external_mem_map {

constexpr uint32_t  sector_size           = STORAGE_SECTOR_SIZE;
constexpr uint16_t  max_sectors_amount    = STORAGE_MAX_SECTORS_AMOUNT;
constexpr uint8_t   sector_rewrite_limit  = STORAGE_SECTOR_REWRITE_LIMIT;

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
