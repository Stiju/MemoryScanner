#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

struct MemoryRegion {
	uint8_t* begin;
	uint8_t* end;
	MemoryRegion(uint8_t* b, uint8_t* e) : begin(b), end(e) {}
};

using MemoryRegions = std::vector<MemoryRegion>;

union DataValue {
	int8_t int8;
	int16_t int16;
	int32_t int32;
	int64_t int64;
	float float_value;
	double double_value;
	void* pointer;
};

struct MemoryResult {
	uint8_t* address;
	DataValue value;
	MemoryResult(uint8_t* adr) : address(adr) {}
	MemoryResult(uint8_t* adr, DataValue v) : address(adr), value(v) {}
	friend bool operator<(const MemoryResult& element, uint8_t* value) { return element.address < value; }
};

using MemoryResults = std::vector<MemoryResult>;

void merge_memory_regions(MemoryRegions& regions);