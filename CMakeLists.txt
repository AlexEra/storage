cmake_minimum_required(VERSION 3.22)

# Test CMakeLists

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_PROJECT_NAME storage)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(${CMAKE_PROJECT_NAME})

enable_language(CXX C ASM)

set(TEMPLATE_STORAGE_TEST)

if(TEMPLATE_STORAGE_TEST)
    add_definitions(-DTEMPLATE_STORAGE_TEST)
endif()

# Storage memory settings
add_definitions(
    -DSTORAGE_SECTOR_SIZE=7
    -DSTORAGE_MAX_SECTORS_AMOUNT=10
    -DSTORAGE_SECTOR_REWRITE_LIMIT=1
)

if(TEMPLATE_STORAGE_TEST)
    add_executable(${CMAKE_PROJECT_NAME}
        tests.cpp
    )
else()
    add_executable(${CMAKE_PROJECT_NAME}
        tests.cpp
        storage.cpp
    )
endif()
