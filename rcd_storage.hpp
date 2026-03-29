#include <stdint.h>
#include <functional>


typedef union {
  struct  {
    uint32_t sector_num: 16;
    uint32_t data_offset: 16;
  };
  uint8_t addr[4];
} parameter_address_t;

#pragma pack(push, 1)
typedef struct {
  uint32_t name_id;
  parameter_address_t parameter_address;
  uint8_t parameter_type;
} parameter_placement_t;
#pragma pack(pop)

template <typename T>
struct parameter_data_t {
  T value;
};

constexpr size_t finish = 1024 / sizeof(parameter_placement_t); // FIXME: 4096

class Storage {
public:
  enum class rw_status {
    WRITE_PARAM_ERROR=-8,
    READ_FAILED=-7,
    CAN_NOT_FIND_END=-6,
    WRITE_ADDR_ERROR=-5,
    NOT_ENOUGH_SPACE=-4,
    BUFFER_IS_NOT_SET=-3,
    PARAMETER_NOT_FOUND=-2,
    TYPE_MISMATCH=-1,
    OK=0,
  };

  // Methods
  Storage() { }
  Storage(uint8_t *p_addresses_buffer);
  void set_buffer(uint8_t *p_addresses_buffer);

  template <typename T> // XXX: может, использовать указатель на void?
  rw_status parameter_set(
    const char *p_parameter_name,
    const uint8_t parameter_type,
    T new_value
  );

  template <typename T> // XXX: может, использовать указатель на void?
  rw_status parameter_get(const char *p_parameter_name, T *p_param);

  bool get_init_info(void);

  template <typename T> // XXX: может, использовать указатель на void?
  rw_status parameter_add(
    const char *p_parameter_name,
    const uint8_t parameter_type,
    T new_value
  );

  rw_status parameter_remove(const char *p_parameter_name);

public:
  // Methods
  std::function<uint32_t(const char *)> compute_name_id {
    [] (const char *) { return 0; }
  };
  std::function<bool(parameter_placement_t &, uint8_t *, uint8_t)>
    write_parameter {
      [] (parameter_placement_t &, uint8_t *, uint8_t) { return false; }
  };
  std::function<bool(parameter_placement_t &, uint8_t *, uint8_t)>
    read_parameter {
      [] (parameter_placement_t &, uint8_t *, uint8_t) { return false; }
  };
  std::function<bool(uint8_t *, size_t, size_t)> read_bytes_from_ext_mem{nullptr};
  std::function<bool(uint8_t *, size_t)> write_bytes_to_ext_mem{nullptr};
  std::function<bool(void)> clean_all{nullptr};

private:
  // Methods
  uint16_t find_parameter_index(uint32_t name_id, bool *p_status);

  // Attributes
  parameter_placement_t *p_params_addresses{nullptr};
  int16_t end_of_placements_list{-1};
  uint16_t data_offset_sum{0}; // FIXME: эта переменная нужна только при вызове метода добавления параметра, значит, она может быть временной
};

template <typename T>
Storage::rw_status Storage::parameter_get(const char *p_parameter_name, T *p_param) {
  if (p_params_addresses == nullptr) {
    return rw_status::BUFFER_IS_NOT_SET;
  }
  bool status;
  uint32_t name_id = compute_name_id(p_parameter_name);
  uint16_t offset = find_parameter_index(name_id, &status);
  if (!status) {
    return rw_status::PARAMETER_NOT_FOUND;
  }
  if (sizeof(*p_param) >= p_params_addresses[offset].parameter_type) {
    status = read_parameter(
      p_params_addresses[offset],
      (uint8_t *) p_param,
      p_params_addresses[offset].parameter_type
    );
    if (status) {
      return rw_status::OK;
    }
    return rw_status::READ_FAILED;
  }
  return rw_status::TYPE_MISMATCH;
}

template <typename T>
Storage::rw_status Storage::parameter_add(
  const char *p_parameter_name,
  const uint8_t parameter_type,
  T new_value
) {
  if (p_params_addresses == nullptr) {
    return rw_status::BUFFER_IS_NOT_SET;
  }
  if (end_of_placements_list == (finish - 1)) {
    return rw_status::NOT_ENOUGH_SPACE;
  }
  parameter_placement_t tmp;
  // get name ID to save it to the external memory
  uint32_t name_id = compute_name_id(p_parameter_name);
  if (!get_init_info()) {
    return rw_status::CAN_NOT_FIND_END;
  }

  // update parameters info
  if (end_of_placements_list != -1) {
    p_params_addresses[++end_of_placements_list].parameter_address.data_offset = 
      data_offset_sum;
  } else {
    end_of_placements_list = 0;
    p_params_addresses[end_of_placements_list].parameter_address.data_offset = 0;
  }
  p_params_addresses[end_of_placements_list].name_id = name_id;
  p_params_addresses[end_of_placements_list].parameter_address.sector_num = 1;
  p_params_addresses[end_of_placements_list].parameter_type = parameter_type;

  // backup parameter address before sorting to get correct data for writing it's value
  tmp = p_params_addresses[end_of_placements_list];
  // sort
  if (end_of_placements_list > 0) {
    std::sort(
      p_params_addresses,
      p_params_addresses + end_of_placements_list + 1,
      [] (const parameter_placement_t &first, const parameter_placement_t &second) {
        return first.name_id < second.name_id;
      }
    );
  }

  // write to external memory
  if (
    !write_bytes_to_ext_mem(
      (uint8_t *) p_params_addresses,
      (end_of_placements_list + 1) * sizeof(parameter_placement_t)
    )
  ) {
    return rw_status::WRITE_ADDR_ERROR;
  }

  // write parameter value
  if (write_parameter(tmp, (uint8_t *) &new_value, sizeof(new_value))) {
    return rw_status::OK;
  }
  return rw_status::WRITE_PARAM_ERROR;
}
