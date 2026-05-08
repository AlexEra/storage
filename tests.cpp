#include <iostream>
#include <assert.h>
#include <algorithm>
#include "storage.hpp"

typedef struct __attribute__((__packed__)) test_data_t {
  uint8_t val_0;
  uint8_t val_1;
  uint16_t rewrite_counter;
  uint8_t crc8;
} test_data_t;

namespace fs = flash_storage;
uint8_t test_mem[sizeof(test_data_t)][fs::sector_size];

int main() {
  fs::Storage st;
  fs::Storage::rw_status status;
  test_data_t param_0, param_1;
  external_mem_map::parameter_metadata_t mdata_0 {
    .start_sector = 0,
    .end_sector = 3,
    .current_sector = 0,
    .data_offset = 0,
    .data_size = sizeof(test_data_t),
    .sector_rewrite_counter = 0
  },
  mdata_1 {
    .start_sector = 4,
    .end_sector = 8,
    .current_sector = 4,
    .data_offset = 0,
    .data_size = sizeof(test_data_t),
    .sector_rewrite_counter = 0
  };

  st.set_erase_all([&] {
    for (auto &r : test_mem) {
      for (auto &c : r) {
        c = 0xFF;
      }
    }
    return true;
  });

  st.set_compute_crc8([&] (uint8_t *ptr_arr, uint8_t sz) {
    uint8_t res{0};
    for (uint8_t i{0}; i < sz; i++) {
      res ^= ptr_arr[i];
    }
    return res;
  });

  st.set_erase_sectors([&] (uint32_t sector_num, uint32_t sectors_to_erase) {
    if (sector_num >= 5) {
      return false;
    }
    uint32_t s_lim = sectors_to_erase + sector_num;
    for (uint32_t s{sector_num}; s < s_lim; s++) {
      if (s >= 5) {
        break;
      }
      for (auto &c : test_mem[s]) {
        c = 0xFF;
      }
    }
    return true;
  });

  st.set_read_bytes([&] (uint8_t sec_num, uint32_t offs, uint8_t *ptr_buf, size_t buf_size) {
    if (sec_num >= 5) {
      return false;
    } else if (offs >= (fs::sector_size - sizeof(test_data_t))) {
      return false;
    } else if (buf_size > sizeof(test_data_t)) {
      return false;
    }
    memcpy(ptr_buf, test_mem[sec_num], buf_size);
    return true;
  });

  st.set_write_bytes([&] (uint8_t sector_number, uint32_t offset, uint8_t *p_buf, size_t buf_size) {
    if (
      (sector_number > external_mem_map::max_sectors_amount) ||
      (offset > external_mem_map::sector_size) ||
      (p_buf == nullptr) || (buf_size > external_mem_map::sector_size)
    ) {
      return false;
    }
    memcpy(test_mem[sector_number], p_buf, buf_size);
    return true;
  });

  // TODO: firstly, test basic functions
  // TODO: secondly, test Storage class methods

  // clean memory - set 0xFF
  for (auto &raw : test_mem) {
    for (auto &value : raw) {
      value = 0xFF;
    }
  }

  status = st.read_data_structure(&mdata_0, (uint8_t *) &param_0);
  std::cout << (int) status << '\n';
  if (status == fs::Storage::rw_status::NO_DATA) {
    // set default values
    param_0.val_0 = 66;
    param_0.val_1 = 42;
    param_0.rewrite_counter = 0;
  } else {
    std::cout << "Something went wrong\n";
  }

  status = st.read_data_structure(&mdata_1, (uint8_t *) &param_1);
  std::cout << (int) status << '\n';
  if (status == fs::Storage::rw_status::NO_DATA) {
    // set default values
    param_1.val_0 = 146;
    param_1.val_1 = 69;
    param_1.rewrite_counter = 0;
  } else {
    std::cout << "Something went wrong\n";
  }

  return 0;
}
