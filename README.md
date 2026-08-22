# storage
Interface-like class to work with external flash memory

# Intro
This class works with only one external flash and doesn't use any file system. The main goal is to use it with MCU, that doesn't have a lot of RAM and internal flash to work with file system. Class operates over bytes and their metadata instead.

Data is saved to external flash, however metadata is placed only in RAM.

Class uses sector rewrite limit to use available sectors by several times. Available sectors are set by developer in the code (see `external_memory_mapping.hpp` and _section below_).

Data can contain or small structures either a big but less than sector size.

Functions to communicate with external flash (read, write, erasing) should be defined by developer, also CRC8 computing function.

# External flash settings and metadata
The memory mapping is described in `external_memory_mapping.hpp`.
This file contains sector size, max sectors, also constant for setting sector rewrite limit.

Besides, this file contains structure type for keeping metadata about data, that is saved to external flash memory.
This structure consists of start and end sectors of data to be saved, current sector, where data is placed, data offset in current sector,
data size, current sector rewrite counter to manage sector using, and finally flag `data_is_read_flag`, to understand, that data is already read or not.

# Data class
The data should be represented as child of `base_data_t` class (structure).
For example
```cpp
// Packed attribute is optional, but helps to compress bytes
struct __attribute__((packed)) my_data_struct_t: public base_data_t {
  int val_0;
  float val_1;
}
```

`base_data_t` contains current rewrite counter of flash cells to reuse them, also crc8 value to check integrity. CRC8 is optional and can be omitted, but this variable is used for checking in read method, that is defined by developer. It should be noticed that crc8 value is first followed by data rewrite counter. 

# Using principles (_limits_ at the same time)
0. metadata should be filled in the code, i.e. start sector, end sector, etc.;
1. set read, write, erase, and CRC8 computing functions (there function pointer or lambda can be used);
2. `read_data_structure` should be called to set up the metadata and external flash;
3. finally, `write_data_structure` can be called to write new data.
