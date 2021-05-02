#define __NO_REDEF_NEW
#include "Common.hpp"

#ifdef DEBUG_MEMORY

#include <unordered_map>
#include <cassert>

struct Allocation
{
	size_t size;
	const char* file;
	int line;
	Allocation(size_t size, const char* file, int line)
		: size(size), file(file), line(line)
	{}
};

static std::unordered_map<uint64_t, Allocation> Allocations;

void* TrackNew(void* ptr, size_t size, const char* file, int line)
{
	Allocations.insert(std::pair<uint64_t, Allocation>((uint64_t)ptr, Allocation(size, file, line)));
	return ptr;
}
void TrackDelete(void* ptr)
{
	Allocations.erase((uint64_t)ptr);
}

void* operator new(const size_t size, const char* file, int line)
{
	void* ptr = std::malloc(size);
	TrackNew(ptr, size, file, line);
	return ptr;
}
void* operator new[](const size_t size, const char* file, int line)
{
	void* ptr = std::malloc(size);
	TrackNew(ptr, size, file, line);
	return ptr;
}
void operator delete(void* ptr)
{
	TrackDelete(ptr);
	std::free(ptr);
}
void operator delete(void* ptr, size_t s)
{
	TrackDelete(ptr);
	std::free(ptr);
}
void operator delete[](void* ptr)
{
	TrackDelete(ptr);
	std::free(ptr);
}
void operator delete[](void* ptr, size_t s)
{
	TrackDelete(ptr);
	std::free(ptr);
}

#endif // DEBUG_MEMORY

uint32_t Memory::TotalAllocated()
{
#ifdef DEBUG_MEMORY
	uint32_t totalAllocated = 0;
	for (const auto& allocation : Allocations)
	{
		totalAllocated += allocation.second.size;
	}

	return totalAllocated;
#else // DEBUG_MEMORY
	return 0;
#endif // DEBUG_MEMORY
}
