#ifndef COMMON_H
#define COMMON_H

// Standard Library Includes
#include <cstdint>
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <vector>


// Project-wide constants
const uint32_t PAGE_SIZE = 4096;
const uint32_t TABLE_MAX_PAGES = 100;

#endif // COMMON_H