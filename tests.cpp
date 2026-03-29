#include <iostream>
#include <assert.h>
#include <algorithm>
#include "rcd_switchers.hpp"
#include "rcd_analog.hpp"
#include "rcd_leds.hpp"
#include "ws2812b_static.hpp"
#include "rcd_storage.hpp"
#include "rcd_filters.hpp"


// #define TEST_DIGITALS
// #define TEST_SWITCHERS_EMULATOR
// #define TEST_ANALOGS
// #define TEST_LEDS
#define WS2812B_BUFFER_TEST
// #define TEST_STORAGE_CLASS
// #define TEST_FILTERS

#if defined(TEST_DIGITALS)

int main() {
  using namespace DigitalControls;

  rcd_button simple_button;
  rcd_button_with_led button_with_led;
  rcd_2_pose_switch switch_2p;
  rcd_3_pose_switch switch_3p;

  button_with_led.led_off = [] (void) {
      std::cout << "LED OFF\n";
  };
  button_with_led.led_on = [] (void) {
      std::cout << "LED ON\n";
  };

  std::cout << "Start:\n";
  std::cout << "Simple button: " << simple_button.rc_value << '\n';
  std::cout << "Button with LED: " << button_with_led.rc_value << '\n';
  std::cout << "2-pose: " << switch_2p.rc_value << '\n';
  std::cout << "3-pose: " << switch_3p.rc_value << '\n';
  
  std::cout << "\nUpdate states:\n";
  simple_button.set_state(1);
  button_with_led.set_state(1);
  switch_2p.set_state(1);
  switch_3p.set_state(1, 0);

  std::cout << "\nAfter 1st states updating:\n";
  std::cout << "Simple button: " << simple_button.rc_value << '\n';
  std::cout << "Button with LED: " << button_with_led.rc_value << '\n';
  std::cout << "2-pose: " << switch_2p.rc_value << '\n';
  std::cout << "3-pose: " << switch_3p.rc_value << '\n';

  std::cout << "\nUpdate states:\n";
  simple_button.set_state(0);
  button_with_led.set_state(0);
  switch_2p.set_state(0);
  switch_3p.set_state(0, 1);

  std::cout << "\nAfter 2nd states updating:\n";
  std::cout << "Simple button: " << simple_button.rc_value << '\n';
  std::cout << "Button with LED: " << button_with_led.rc_value << '\n';
  std::cout << "2-pose: " << switch_2p.rc_value << '\n';
  std::cout << "3-pose: " << switch_3p.rc_value << '\n';

  std::cout << "\nUpdate states:\n";
  simple_button.invert_state_to_rc_value();
  button_with_led.invert_state_to_rc_value();
  switch_2p.invert_state_to_rc_value();
  // simple_button.pressed_is_max_rc = false;
  // button_with_led.pressed_is_max_rc = false;
  // switch_2p.pressed_is_max_rc = false;
  // simple_button.set_state(0);
  // button_with_led.set_state(0);
  // switch_2p.set_state(0);
  switch_3p.set_state(0, 0);

  std::cout << "\nAfter 3rd states updating:\n";
  std::cout << "Simple button: " << simple_button.rc_value << '\n';
  std::cout << "Button with LED: " << button_with_led.rc_value << '\n';
  std::cout << "2-pose: " << switch_2p.rc_value << '\n';
  std::cout << "3-pose: " << switch_3p.rc_value << '\n';

  std::cout << "\nUpdate 3-p switch pins roles:\n";
  using pins_3psw_roles = DigitalControls::rcd_3_pose_switch;
  switch_3p.set_pos_to_rc_value(
    pins_3psw_roles::MINIMUM_RC,
    pins_3psw_roles::MEDIUM_RC,
    pins_3psw_roles::MAXIMUM_RC
  );
  // switch_3p.set_state(0, 0);

  std::cout << "\nCheck state:\n";
  std::cout << "3-pose: " << switch_3p.rc_value << '\n';

  std::cout << "\nChange 3-pose switch state:\n";
  switch_3p.set_state(1, 0);

  std::cout << "\nCheck state:\n";
  std::cout << "3-pose: " << switch_3p.rc_value << '\n';

  std::cout << "\nChange 3-pose switch state:\n";
  switch_3p.set_state(0, 1);

  std::cout << "\nCheck state:\n";
  std::cout << "3-pose: " << switch_3p.rc_value << '\n';

  std::function<void(void)> ttt{nullptr};
  try {
    ttt();
    std::cout << "Successful calling\n";
  } catch (std::bad_function_call e) {
    std::cout << "Failed calling\n";
  }

  return 0;
}

#elif defined(TEST_ANALOGS)

int main() {
  using namespace AnalogControls;
  rcd_battery bat;
  rcd_joystick_axis joy;

  bat.set_max(4095);
  bat.set_min(2048);

  joy.set_max(3200);
  joy.set_mid(2600);
  joy.set_min(2000);

  std::cout << "1. Set raw values\n";
  bat.raw_signal = 5000;
  joy.raw_signal = 2149;

  std::cout << "Transform values\n";
  std::cout << "BAT: " << (int) bat.compute_charge_level() << '\n'; // (4095 - 2048) / 2047 * 100 = 100
  assert(bat.charge_level == 100);
  std::cout << "Joy: " << joy.map_raw_to_rc_value() << "\n\n"; // 1000 + 149 / 600 * 500 = 1124
  assert(joy.rc_value == 1124);

  std::cout << "2. Set raw values\n";
  bat.raw_signal = 3000;
  joy.raw_signal = 3199;

  std::cout << "Transform values\n";
  std::cout << "BAT: " << (int) bat.compute_charge_level() << '\n'; // (3000 - 2048) / 2047 * 100 = 46
  assert(bat.charge_level == 46);
  std::cout << "Joy: " << joy.map_raw_to_rc_value() << "\n\n"; // 1500 + 599 / 600 * 500 = 1999
  assert(joy.rc_value == 1999);

  return 1;
}

#elif defined(TEST_LEDS)

int main() {
  using namespace LEDS;

  led_array<uint32_t, 2> leds;
  for (auto &led: leds.leds) {
    led.set_state = [] (bool s) {
      std::cout << s << '\n';
    };
  }
  leds.delay_ms = [] (uint32_t d) {
    std::cout << "Delay: " << d << '\n';
  };
  
  leds.show_start(1);
  std::cout << '\n';
  leds.show_shutdown(666);

  return 1;
}

#elif defined(TEST_SWITCHERS_EMULATOR)
int main() {
  using namespace DigitalControls;
  rcd_linked_buttons lb;
  /*lb.buttons[0].led_on = [] () {
    std::cout << "\nLED 0 is on\n";
  };
  lb.buttons[1].led_on = [] () {
    std::cout << "\nLED 1 is on\n";
  };
  lb.buttons[2].led_on = [] () {
    std::cout << "\nLED 2 is on\n";
  };
  lb.buttons[0].led_off = [] () {
    std::cout << "\nLED 0 is off\n";
  };
  lb.buttons[1].led_off = [] () {
    std::cout << "\nLED 1 is off\n";
  };
  lb.buttons[2].led_off = [] () {
    std::cout << "\nLED 2 is off\n";
  };*/
  // тесты в обычном режиме
  std::cout << "\nOrdinary mode test\n";
  std::cout << "------------------------------------------------------\n";
  lb.change_button_mode(0, 1);
  std::cout << "BTN_0 init state: " << lb.buttons[0].state << '\n';
  assert(lb.buttons[0].state == 0);
  lb.update_state(0, 1);
  std::cout << "BTN_0 updated state: " << lb.buttons[0].state << '\n';
  assert(lb.buttons[0].state == 1);
  lb.update_state(0, 0);
  std::cout << "BTN_0 updated state: " << lb.buttons[0].state << '\n';
  assert(lb.buttons[0].state == 0);

  // тесты эмуляции двухпозиционного тумблера
  std::cout << "\n2p mode test\n";
  std::cout << "------------------------------------------------------\n";
  lb.change_button_mode(0, 2);
  assert(lb.buttons[0].state == 0);
  std::cout << "BTN_0 init state: " << lb.buttons[0].state << '\n';
  lb.update_state(0, 1); // on
  assert(lb.buttons[0].state == 1);
  std::cout << "BTN_0 updated state: " << lb.buttons[0].state << '\n';
  lb.update_state(0, 0); // всё равно on
  assert(lb.buttons[0].state == 1);
  std::cout << "BTN_0 updated state: " << lb.buttons[0].state << '\n';
  lb.update_state(0, 1); // off
  assert(lb.buttons[0].state == 0);
  std::cout << "BTN_0 updated state: " << lb.buttons[0].state << '\n';

  // тесты эмуляции трёхпозиционного тумблера
  std::cout << "\n3p mode test\n";
  std::cout << "------------------------------------------------------\n";
  lb.change_button_mode(0, 3);
  std::cout << "rc: " << lb.common_rc_value << '\n';
  assert(lb.common_rc_value == 1000);
  lb.update_state(0, 1);
  std::cout << "rc: " << lb.common_rc_value << '\n';
  assert(lb.common_rc_value == 2000);
  lb.update_state(0, 0);
  assert(lb.common_rc_value == 2000);
  lb.update_state(1, 1);
  std::cout << "rc: " << lb.common_rc_value << '\n';
  assert(lb.common_rc_value == 1500);
  lb.update_state(1, 0);
  assert(lb.common_rc_value == 1500);
  lb.update_state(2, 1);
  std::cout << "rc: " << lb.common_rc_value << '\n';
  assert(lb.common_rc_value == 1000);
  lb.update_state(2, 0);
  assert(lb.common_rc_value == 1000);
  return 0;
}

#elif defined(WS2812B_BUFFER_TEST)

WS2812Bstatic<uint8_t, 2, 1, 0, 2> rgb;

void print_rgb(void) {
  for (auto i: rgb.buffer) {
    std::cout << (int) i << ' ';
  }
  std::cout << '\n';
}

int main() {
  rgb.reset_all();
  print_rgb();
  rgb.set_green_to_led(255, 0);
  print_rgb();
  rgb.set_white_to_led(129, 1);
  print_rgb();
  rgb.reset_led(1);
  print_rgb();
  rgb.reset_all();
  print_rgb();
  return 0;
}

#elif defined(TEST_STORAGE_CLASS)

#include <string.h>

constexpr size_t U8_DATA_COUNT = 2;
constexpr size_t U16_DATA_COUNT = 1;
constexpr size_t U32_DATA_COUNT = 1;
constexpr size_t TOTAL_PARAMS = U8_DATA_COUNT + U16_DATA_COUNT + U32_DATA_COUNT;
constexpr size_t TOTAL_BYTES = U8_DATA_COUNT + (U16_DATA_COUNT << 1) + (U32_DATA_COUNT << 2);

uint8_t addresses[TOTAL_PARAMS * sizeof(parameter_placement_t)];
uint8_t data[TOTAL_BYTES];
uint8_t buffer[1024];

int main() {
  using st = Storage::rw_status;

  uint8_t param_0, param_1;
  uint16_t param_2;
  uint32_t param_3;
  st status;
  
  Storage s(buffer);

  // init lambdas
  s.read_bytes_from_ext_mem = [] (uint8_t *ptr, size_t bytes_count, size_t index) {
    /*if (bytes_count > sizeof(parameter_placement_t)) {
      return false;
    }*/
    memcpy(ptr, &addresses[index * sizeof(parameter_placement_t)], bytes_count);
    return true;
  };
  s.compute_name_id = [] (const char *name_str) {
    uint32_t ret{0};
    size_t sz = strlen(name_str);
    for (uint8_t i{0}; i < sz; i++)  {
      ret ^= name_str[i];
    }
    return ret;
  };
  s.read_parameter = [] (
    parameter_placement_t &placement,
    uint8_t *ptr,
    uint8_t count
  ) {
    memcpy(ptr, &data[placement.parameter_address.data_offset], count);
    return true;
  };
  s.write_bytes_to_ext_mem = [] (uint8_t *ptr, size_t count) {
    if (count > sizeof(addresses)) {
      std::cout << "OVERBYTES: " << count << '\n';
      return false;
    }
    memset(addresses, 0xFF, sizeof(addresses));
    memcpy(addresses, ptr, count);
    return true;
  };
  s.write_parameter = [] (
    parameter_placement_t &placement,
    uint8_t *ptr,
    uint8_t count
  ) {
    if (placement.parameter_address.data_offset > TOTAL_BYTES) {
      return false;
    }
    memcpy(&data[placement.parameter_address.data_offset], ptr, count);
    return true;
  };

  // prepare `addresses` buffer
  memset(addresses, 0xFF, sizeof(addresses));

  // Order: u8, u8, u16, u32
  /////////////////////////////////////////////// Add param
  std::cout << "Add u8\n";
  status = s.parameter_add<uint8_t>("PARAM0", sizeof(uint8_t), 66);
  if (status != st::OK) {
    std::cout << "Error: " << (int) status << '\n';
  }
  status = s.parameter_get("PARAM0", &param_0);
  if (status != st::OK) {
    std::cout << "Error: " << (int) status << '\n';
  } else {
    std::cout << "Got param_0: "  << (int) param_0 << '\n';
  }
  parameter_placement_t *ptr_placement = (parameter_placement_t *) addresses;
  std::cout << "Name ID: " << ptr_placement->name_id << std::endl;
  std::cout << "Data offset: " << ptr_placement->parameter_address.data_offset << std::endl;
  std::cout << "Sector number: " << ptr_placement->parameter_address.sector_num << std::endl;
  assert(param_0 == 66);

  /////////////////////////////////////////////// Add param
  std::cout << "\nAdd second u8\n";
  status = s.parameter_add<uint8_t>("PARAM1", sizeof(uint8_t), 42);
  if (status != st::OK) {
    std::cout << "Error: " << (int) status << '\n';
  }
  status = s.parameter_get("PARAM1", &param_1);
  if (status != st::OK) {
    std::cout << "Error: " << (int) status << '\n';
  } else {
    std::cout << "Got param_1: "  << (int) param_1 << '\n';
  }
  std::cout << "Name ID: " << ptr_placement->name_id << std::endl;
  std::cout << "Data offset: " << ptr_placement->parameter_address.data_offset << std::endl;
  std::cout << "Sector number: " << ptr_placement->parameter_address.sector_num << std::endl;
  assert(param_1 == 42);

  /////////////////////////////////////////////// Add param
  std::cout << "\nAdd u16\n";
  status = s.parameter_add<uint16_t>("PARAM3", sizeof(uint16_t), 65534);
  if (status != st::OK) {
    std::cout << "Error: " << (int) status << '\n';
  }
  status = s.parameter_get("PARAM3", &param_2);
  if (status != st::OK) {
    std::cout << "Error: " << (int) status << '\n';
  } else {
    std::cout << "Got param_2: "  << (int) param_2 << '\n';
  }
  std::cout << "Name ID: " << ptr_placement->name_id << std::endl;
  std::cout << "Data offset: " << ptr_placement->parameter_address.data_offset << std::endl;
  std::cout << "Sector number: " << ptr_placement->parameter_address.sector_num << std::endl;
  assert(param_2 == 65534);

  /////////////////////////////////////////////// Add param
  std::cout << "\nAdd u32\n";
  status = s.parameter_add<uint32_t>("PARAM2", sizeof(uint32_t), 666111);
  if (status != st::OK) {
    std::cout << "Error: " << (int) status << '\n';
  }
  status = s.parameter_get("PARAM2", &param_3);
  if (status != st::OK) {
    std::cout << "Error: " << (int) status << '\n';
  } else {
    std::cout << "Got param_3: "  << (int) param_3 << '\n';
  }
  std::cout << "Name ID: " << (ptr_placement + 1)->name_id << std::endl;
  std::cout << "Data offset: " << (ptr_placement + 1)->parameter_address.data_offset << std::endl;
  std::cout << "Sector number: " << (ptr_placement + 1)->parameter_address.sector_num << std::endl;
  assert(param_3 == 666111);

  /////////////////////////////////////////////// print all
  std::cout << std::endl;
  uint32_t param_value;
  for (
    ; ptr_placement < (((parameter_placement_t *)addresses) + TOTAL_PARAMS);
    ptr_placement++
  ) {
    if (ptr_placement->parameter_type == 0xFF) {
      break;
    }
    std::cout << "Name ID: " << ptr_placement->name_id << ", "
    << "Type: " << (int) ptr_placement->parameter_type << ", ";
    param_value = 0;
    memcpy(
      (uint8_t *) &param_value,
      data + ptr_placement->parameter_address.data_offset,
      ptr_placement->parameter_type
    );
    std::cout << "Data: " << (int) param_value << '\n';
  }

  /////////////////////////////////////////////// delete PARAM0
  std::cout << "\nDelete PARAM1\n";
  status = s.parameter_remove("PARAM1");
  if (status != st::OK) {
    std::cout << "Error: " << (int) status << '\n';
  }

  /////////////////////////////////////////////// print all
  std::cout << std::endl;
  for (
    ptr_placement = (parameter_placement_t *) addresses;
    ptr_placement < (((parameter_placement_t *)addresses) + TOTAL_PARAMS);
    ptr_placement++
  ) {
    if (ptr_placement->parameter_type == 0xFF) {
      break;
    }
    std::cout << "Name ID: " << ptr_placement->name_id << ", "
    << "Type: " << (int) ptr_placement->parameter_type << ", ";
    param_value = 0;
    memcpy(
      (uint8_t *) &param_value,
      data + ptr_placement->parameter_address.data_offset,
      ptr_placement->parameter_type
    );
    std::cout << "Data: " << (int) param_value << '\n';
  }

  /////////////////////////////////////////////// delete PARAM3
  std::cout << "\nDelete PARAM3\n";
  status = s.parameter_remove("PARAM3");
  if (status != st::OK) {
    std::cout << "Error: " << (int) status << '\n';
  }

  /////////////////////////////////////////////// print all
  std::cout << std::endl;
  for (
    ptr_placement = (parameter_placement_t *) addresses;
    ptr_placement < (((parameter_placement_t *)addresses) + TOTAL_PARAMS);
    ptr_placement++
  ) {
    if (ptr_placement->parameter_type == 0xFF) {
      break;
    }
    std::cout << "Name ID: " << ptr_placement->name_id << ", "
    << "Type: " << (int) ptr_placement->parameter_type << ", ";
    param_value = 0;
    memcpy(
      (uint8_t *) &param_value,
      data + ptr_placement->parameter_address.data_offset,
      ptr_placement->parameter_type
    );
    std::cout << "Data: " << (int) param_value << '\n';
  }

  /////////////////////////////////////////////// erase all
  std::cout << "\nErase\n";
  s.clean_all = [] {
    memset(addresses, 0xFF, sizeof(addresses));
    memset(data, 0xFF, sizeof(data));
    return true;
  };
  s.clean_all();

  /////////////////////////////////////////////// print all
  std::cout << std::endl;
  for (
    ptr_placement = (parameter_placement_t *) addresses;
    ptr_placement < (((parameter_placement_t *)addresses) + TOTAL_PARAMS);
    ptr_placement++
  ) {
    if (ptr_placement->parameter_type != 0xFF) {
      std::cout << "Name ID: " << ptr_placement->name_id << ", "
      << "Type: " << (int) ptr_placement->parameter_type << ", ";
      param_value = 0;
      memcpy(
        (uint8_t *) &param_value,
        data + ptr_placement->parameter_address.data_offset,
        ptr_placement->parameter_type
      );
      std::cout << "Data: " << (int) param_value << '\n';
    }
  }

  return 0;
}

#elif defined(TEST_FILTERS)

int main() {
  DigitalFilters::RunningAverageFilter<int16_t, 4> ff;
  ff.start();
  std::cout << ff(4) << '\n';
  std::cout << ff(1) << '\n';
  std::cout << ff(5) << '\n';
  std::cout << ff(3) << '\n';
  std::cout << ff(6) << '\n';
  std::cout << ff(6) << '\n';
  return 0;
}

#endif
