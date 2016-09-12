#pragma once

#include <vector>

struct MemoryRegion {
	void* begin;
	void* end;
	MemoryRegion(void* b, void* e) : begin(b), end(e) {}
};

using MemoryRegions = std::vector<MemoryRegion>;

struct MemoryResult {
	void* address;
	int value;
	MemoryResult(void* adr, int v) : address(adr), value(v) {}
};

using MemoryResults = std::vector<MemoryResult>;

size_t sys_get_error();
void sys_set_process_id(size_t process_id);
bool sys_open_process(size_t process_id);
bool sys_close_process();
bool sys_seek_memory(void* address);
bool sys_read_memory(void* address, void* buffer, size_t size, size_t* read);
bool sys_write_memory(void* address, const void* buffer, size_t size, size_t* written);


MemoryRegions sys_memory_regions();