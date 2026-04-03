#include <iostream>
#include <assert.h>
#include <algorithm>
#include "storage.hpp"

namespace fs = flash_storage;
uint8_t test_mem[5][fs::sector_size];

int main() {
  fs::Storage st;

  st.set_max_sectors(5);

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
    } else if (offs >= (fs::sector_size - sizeof(main_flash_data_t))) {
      return false;
    } else if (buf_size > sizeof(main_flash_data_t)) {
      return false;
    }
    memcpy(ptr_buf, test_mem[sec_num], buf_size);
    return true;
  });

  st.set_write_bytes(); // TOOD: implement

  // TODO: firstly, test basic functions
  // TODO: secondly, test Storage class methods

  return 0;
}
