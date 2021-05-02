/// This implementation of the Allocator will track allocations made by files that
/// include the or Common.hpp file. To enable allocation tracking define DEBUG_MEMORY
/// in Common.hpp.

#ifdef DEBUG_MEMORY
#pragma once
#include <new>

void* operator new(size_t, const char*, int);
void* operator new[](size_t, const char*, int);
void operator delete(void*);
void operator delete(void*, size_t);
void operator delete[](void*);
void operator delete[](void*, size_t);

#ifndef __NO_REDEF_NEW
#define new new(__FILE__, __LINE__)
#endif // __NO_REDEF_NEW
#endif // DEBUG_MEMORY

namespace Memory
{
	uint32_t TotalAllocated();
	//void DumpAllocations();
}