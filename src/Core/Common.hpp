#pragma once

#define NOMINMAX

//#define LOGGER_DO_TRACE
//#define IMGUI_LOGGER_USE_COLORS
//#define MEASURE_MEMORY_CONSUMPTION
//#define PRE_ANIMATE_CAMERA
//#define PROCEDURAL_GRID 1
//#define LEVEL_SET_RANDOMIZE_COLOR

#include <vulkan/vulkan.hpp>

#include <cstdlib>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cctype>
#include <cassert>
#include <utility>
#include <memory>

#if defined(WIN32) || defined(_WIN32) || defined (__WIN32__)
#define PLATFORM_WINDOWS
#ifndef _WIN64
#warning "Using 32-bit Windows OS, this program has not been tested on 32-bit systems."
#endif // _WIN64
#else // defined(WIN32) || defined(_WIN32) || defined (__WIN32__)
#error "OS not supported."
#endif // defined(WIN32) || defined(_WIN32) || defined (__WIN32__)

// TODO: Memory tracking.