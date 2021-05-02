#pragma once

#include <cstdlib>
#include <cstdint>
#include <cassert>

//#define DEBUG_MEMORY

#if defined(WIN32) || defined(_WIN32) || defined (__WIN32__)
#define PLATFORM_WINDOWS
#ifndef _WIN64
#warning "Using 32-bit Windows OS, this has not been tested on 32-bit systems."
#endif // _WIN64
#else // defined(WIN32) || defined(_WIN32) || defined (__WIN32__)
#error "OS not supported."
#endif // defined(WIN32) || defined(_WIN32) || defined (__WIN32__)

#include "Memory/Allocator.hpp"
