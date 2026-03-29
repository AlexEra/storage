#include "rcd_storage.hpp"


Storage::Storage(uint8_t *p_addresses_buffer) {
  set_buffer(p_addresses_buffer);
}

void Storage::set_buffer(uint8_t *p_addresses_buffer) {
  p_params_addresses = 
    reinterpret_cast<parameter_placement_t *>(p_addresses_buffer);
}

template <typename T>
Storage::rw_status Storage::parameter_set(
  const char *p_parameter_name,
  const uint8_t parameter_type,
  T new_value
) {
  if (p_params_addresses == nullptr) {
    return rw_status::BUFFER_IS_NOT_SET;
  }
  bool status;
  uint32_t name_id = compute_name_id(p_parameter_name);
  uint16_t offset = find_parameter_index(name_id, &status);
  if (!status) {
    return rw_status::PARAMETER_NOT_FOUND;
  }
  if (p_params_addresses[offset].parameter_type != parameter_type) {
    return rw_status::TYPE_MISMATCH;
  }
  write_parameter(
    p_params_addresses[offset],
    (uint8_t *) &new_value,
    parameter_type
  ); // FIXME: прежде чем писать, нужно очистить сектор, а до этого - сохранить копию
  return rw_status::OK;
}

uint16_t Storage::find_parameter_index(uint32_t name_id, bool *p_status) {
  if (p_params_addresses == nullptr || end_of_placements_list == -1) {
    *p_status = false;
    return 0;
  }
  int32_t idx_beg{0}, idx_end{end_of_placements_list}, idx_mid = idx_end >> 1;
  while (idx_beg != idx_mid) {
    if (name_id > p_params_addresses[idx_mid].name_id) {
      idx_beg = idx_mid;
    } else if (name_id < p_params_addresses[idx_mid].name_id) {
      idx_end = idx_mid;
    } else {
      *p_status = true;
      return idx_mid;
    }
    idx_mid = ((idx_end - idx_beg) >> 1) + idx_beg;
  }
  if (name_id == p_params_addresses[idx_end].name_id) {
    *p_status = true;
    return idx_end;
  } else if (name_id == p_params_addresses[idx_beg].name_id) {
    *p_status = true;
    return idx_beg;
  }
  *p_status = false;
  return 0;
}

bool Storage::get_init_info(void) {
  if (read_bytes_from_ext_mem == nullptr) {
    return false;
  }
  size_t i{0};
  data_offset_sum = 0;
  for (; i < finish; i++) {
    if (
      read_bytes_from_ext_mem(
        (uint8_t *) &p_params_addresses[i],
        sizeof(parameter_placement_t),
        i
      )
    ) {
      if (
        (p_params_addresses[i].parameter_type == 0xFF)
        || (p_params_addresses[i].parameter_address.sector_num == 0xFF)
        || (i == (finish - 1))
      ) {
        if (i) {
          end_of_placements_list = i - 1;
        }
        return true;
      } else {
        data_offset_sum += p_params_addresses[i].parameter_type;
      }
    }
  }
  if (i == finish) {
    end_of_placements_list = finish - 1;
    return true;
  }
  return false;
}

Storage::rw_status Storage::parameter_remove(const char *p_parameter_name) {
  if (p_params_addresses == nullptr) {
    return rw_status::BUFFER_IS_NOT_SET;
  }
  bool status;
  uint32_t name_id = compute_name_id(p_parameter_name);
  uint16_t offset = find_parameter_index(name_id, &status);
  if (!status) {
    return rw_status::PARAMETER_NOT_FOUND;
  }
  if (end_of_placements_list != -1) {
    // read only written and existing data
    bool read_status = read_bytes_from_ext_mem(
      (uint8_t *) p_params_addresses,
      (end_of_placements_list + 1) * sizeof(parameter_placement_t),
      0
    );
    /* FIXME:
      значения параметров не сдвигаются. Также это потребует изменения data_offset у каждого параметра
      !!!! Отсутствие сдвига приведёт однажды к переполнению при удалении и добавлении параметров
    */
    if (read_status) {
      if (p_params_addresses[offset].name_id == name_id) {
        if (offset != end_of_placements_list) {
          for (uint16_t i{offset}; i < end_of_placements_list; i++) {
            p_params_addresses[i].name_id = p_params_addresses[i + 1].name_id;
            p_params_addresses[i].parameter_type = p_params_addresses[i + 1].parameter_type;
            p_params_addresses[i].parameter_address.data_offset =
              p_params_addresses[i + 1].parameter_address.data_offset;
          }
        }/* else {
          p_params_addresses[offset].name_id = 0xFFFFFFFF;
          p_params_addresses[offset].parameter_type = 0xFF;
          p_params_addresses[offset].parameter_address.data_offset = 0xFF;
          p_params_addresses[offset].parameter_address.sector_num = 0xFF;
        }*/
        end_of_placements_list--;
        // write to external memory
        if (
          write_bytes_to_ext_mem(
            (uint8_t *) p_params_addresses,
            (end_of_placements_list + 1) * sizeof(parameter_placement_t)
          )
        ) {
          return rw_status::OK;
        } else {
          return rw_status::WRITE_ADDR_ERROR;
        }
      }
    } else {
      return rw_status::READ_FAILED;
    }
  }
  return rw_status::PARAMETER_NOT_FOUND;
}
