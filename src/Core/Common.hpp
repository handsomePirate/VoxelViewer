#pragma once

#include <cstdlib>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cctype>
#include <cassert>
#include <utility>

#if defined(WIN32) || defined(_WIN32) || defined (__WIN32__)
#define PLATFORM_WINDOWS
#ifndef _WIN64
#warning "Using 32-bit Windows OS, this has not been tested on 32-bit systems."
#endif // _WIN64
#else // defined(WIN32) || defined(_WIN32) || defined (__WIN32__)
#error "OS not supported."
#endif // defined(WIN32) || defined(_WIN32) || defined (__WIN32__)

// TODO: Memory tracking.