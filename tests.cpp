#include <iostream>
#include <assert.h>
#include <algorithm>
#include "storage.hpp"

namespace fs = flash_storage;

struct __attribute__((__packed__)) test_data_t : public fs::base_data_t {
  uint8_t val_0;
  uint8_t val_1;
};

uint8_t test_mem[fs::max_sectors_amount][sizeof(test_data_t)];

void print_test_mem(void) {
  for (auto &raw : test_mem) {
    for (auto &value : raw) {
      std::cout << (int) value << ' ';
    }
    std::cout << std::endl;
  }
}

bool read(
  uint32_t sec_num, uint32_t offs,
  uint8_t *ptr_buf, size_t buf_size
) {
  if (sec_num >= external_mem_map::max_sectors_amount) {
    return false;
  } else if (offs > (fs::sector_size - sizeof(test_data_t))) {
    return false;
  } else if (buf_size > sizeof(test_data_t)) {
    return false;
  }
  memcpy(ptr_buf, test_mem[sec_num], buf_size);
  return true;
}

bool write(
  uint32_t sector_number, uint32_t offset,
  uint8_t *p_buf, size_t buf_size
) {
  if (
    (sector_number > external_mem_map::max_sectors_amount) ||
    (offset > external_mem_map::sector_size) ||
    (p_buf == nullptr) || (buf_size > external_mem_map::sector_size)
  ) {
    return false;
  }
  memcpy(test_mem[sector_number], p_buf, buf_size);
  return true;
}

bool erase_all(void) {
  for (auto &r : test_mem) {
    for (auto &c : r) {
      c = 0xFF;
    }
  }
  return true;
}

bool erase_sectors(uint32_t sector_num, uint32_t sectors_to_erase) {
  if (sector_num >= external_mem_map::max_sectors_amount) {
    return false;
  }
  uint32_t s_lim = sectors_to_erase + sector_num;
  for (uint32_t s{sector_num}; s < s_lim; s++) {
    if (s >= external_mem_map::max_sectors_amount) {
      break;
    }
    for (auto &c : test_mem[s]) {
      c = 0xFF;
    }
  }
  return true;
}

uint8_t compute_crc8(uint8_t *ptr_arr, uint8_t sz) {
  uint8_t res{0};
  for (uint8_t i{0}; i < sz; i++) {
    res ^= ptr_arr[i];
  }
  return res;
}
#ifdef TEMPLATE_STORAGE_TEST

#endif /* TEMPLATE_STORAGE_TEST */

int main() {
#ifndef TEMPLATE_STORAGE_TEST
  fs::Storage st;
#else
  fs::StructStorage<read, write, erase_sectors, erase_all, compute_crc8> st;
#endif
  fs::Storage::rw_status status;
  test_data_t param_0, param_1;
  external_mem_map::parameter_metadata_t mdata_0 {
    .start_sector = 0,
    .end_sector = 3,
    .current_sector = 0,
    .data_offset = 0,
    .data_size = sizeof(test_data_t),
    .sector_rewrite_counter = 0,
    .data_is_read_flag = false
  },
  mdata_1 {
    .start_sector = 4,
    .end_sector = 5,
    .current_sector = 4,
    .data_offset = 0,
    .data_size = sizeof(test_data_t),
    .sector_rewrite_counter = 0,
    .data_is_read_flag = false
  };

#ifndef TEMPLATE_STORAGE_TEST
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
    if (sector_num >= external_mem_map::max_sectors_amount) {
      return false;
    }
    uint32_t s_lim = sectors_to_erase + sector_num;
    for (uint32_t s{sector_num}; s < s_lim; s++) {
      if (s >= external_mem_map::max_sectors_amount) {
        break;
      }
      for (auto &c : test_mem[s]) {
        c = 0xFF;
      }
    }
    return true;
  });

  st.set_read_bytes([&] (uint8_t sec_num, uint32_t offs, uint8_t *ptr_buf, size_t buf_size) {
    if (sec_num >= external_mem_map::max_sectors_amount) {
      return false;
    } else if (offs > (fs::sector_size - sizeof(test_data_t))) {
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
#endif /* TEMPLATE_STORAGE_TEST */

  // clean memory - set 0xFF
  for (auto &raw : test_mem) {
    for (auto &value : raw) {
      value = 0xFF;
    }
  }

  // read param_0
  status = st.read_data_structure(&mdata_0, &param_0);
  std::cout << (int) status << '\n';
  if (status == fs::Storage::rw_status::NO_DATA) {
    // set default values
    param_0.val_0 = 66;
    param_0.val_1 = 42;
    param_0.rewrite_counter = 0;
  } else {
    std::cout << "Something went wrong\n";
  }

  // read param_1
  status = st.read_data_structure(&mdata_1, &param_1);
  std::cout << (int) status << '\n';
  if (status == fs::Storage::rw_status::NO_DATA) {
    // set default values
    param_1.val_0 = 146;
    param_1.val_1 = 69;
    param_1.rewrite_counter = 0;
  } else {
    std::cout << "Something went wrong\n";
  }

  // write param_0
  status = st.write_data_structure(&mdata_0, &param_0);
  if (status != fs::Storage::rw_status::OK) {
    std::cout << "Error: " << (int) status << '\n';
  } else {
    std::cout << "param_0 was written\n";
  }
  print_test_mem();
  std::cout << '\n';

  // write param_1
  status = st.write_data_structure(&mdata_1, &param_1);
  if (status != fs::Storage::rw_status::OK) {
    std::cout << "Error: " << (int) status << '\n';
  } else {
    std::cout << "param_1 was written\n";
  }
  print_test_mem();
  std::cout << '\n';

  // read param_0 again
  status = st.read_data_structure(&mdata_0, &param_0);
  if (status != fs::Storage::rw_status::OK) {
    std::cout << "Error: " << (int) status << '\n';
  } else {
    std::cout << "param_0 was read again\n";
  }
  std::cout << '\n';

  // read param_1 again
  mdata_1.data_is_read_flag = false; // imitate rebooting
  status = st.read_data_structure(&mdata_1, &param_1);
  if (status != fs::Storage::rw_status::OK) {
    std::cout << "Error: " << (int) status << '\n';
  } else {
    std::cout << "param_1 was read again\n";
  }
  std::cout << '\n';

  // set and write new param_1
  param_1.val_0 = 83;
  param_1.val_1 = 30;
  status = st.write_data_structure(&mdata_1, &param_1);
  if (status != fs::Storage::rw_status::OK) {
    std::cout << "Error: " << (int) status << '\n';
  } else {
    std::cout << "param_1 was written\n";
    print_test_mem();
    std::cout << "Rewrite counter: " << (int) mdata_1.sector_rewrite_counter << '\n';
    std::cout << "Data offset: " << (int) mdata_1.data_offset << '\n';
  }
  std::cout << '\n';

  // read param_1 again
  mdata_1.data_is_read_flag = false; // imitate rebooting
  status = st.read_data_structure(&mdata_1, &param_1);
  if (status != fs::Storage::rw_status::OK) {
    std::cout << "Error: " << (int) status << '\n';
  } else {
    std::cout << "param_1 was read again\n";
    std::cout << "Rewrite counter: " << (int) mdata_1.sector_rewrite_counter << '\n';
  }
  std::cout << '\n';

  // write param_1 again, there it should be written to next sector
  param_1.val_0 = 49;
  status = st.write_data_structure(&mdata_1, &param_1);
  if (status != fs::Storage::rw_status::OK) {
    std::cout << "Error: " << (int) status << '\n';
  } else {
    std::cout << "param_1 was written\n";
    print_test_mem();
    std::cout << "Rewrite counter: " << (int) mdata_1.sector_rewrite_counter << '\n';
    std::cout << "Data offset: " << (int) mdata_1.data_offset << '\n';
  }
  std::cout << '\n';

  // write param_1 again to increase rewrite_counter and to start writing from the beginning
  for (auto i = 0; i < 2; i++) {
    param_1.val_0++;
    status = st.write_data_structure(&mdata_1, &param_1);
    if (status != fs::Storage::rw_status::OK) {
      std::cout << "Error: " << (int) status << '\n';
    } else {
      std::cout << "param_1 was written\n";
      print_test_mem();
      std::cout << "Rewrite counter: " << (int) mdata_1.sector_rewrite_counter << '\n';
      std::cout << "Data offset: " << (int) mdata_1.data_offset << '\n';
    }
    std::cout << '\n';
  }

  return 0;
}
