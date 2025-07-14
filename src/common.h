#ifndef COMMON_H
#define COMMON_H

// Standard library includes used across the project
#include <cstdint>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <memory>    // For std::unique_ptr
#include <stdexcept> // For std::runtime_error
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

// Project-wide constants
const uint32_t PAGE_SIZE = 4096;
const uint32_t TABLE_MAX_PAGES = 100;

#endif // COMMON_H
