#pragma once

#include <vector>
#include <cstdint>

struct MemoryRegion {
	uint8_t* begin;
	uint8_t* end;
	MemoryRegion(uint8_t* b, uint8_t* e) : begin(b), end(e) {}
};

using MemoryRegions = std::vector<MemoryRegion>;

struct MemoryResult {
	uint8_t* address;
	int value;
	MemoryResult(uint8_t* adr, int v) : address(adr), value(v) {}
	friend bool operator<(const MemoryResult& element, uint8_t* value) { return element.address < value; }
};

using MemoryResults = std::vector<MemoryResult>;

void merge_memory_regions(MemoryRegions& regions);