#pragma once

#include "memory.h"

size_t sys_get_error();
void sys_set_process_id(size_t process_id);
bool sys_open_process(size_t process_id);
bool sys_close_process();
bool sys_seek_memory(void* address);
bool sys_read_memory(void* address, void* buffer, size_t size, size_t* read);
bool sys_write_memory(void* address, const void* buffer, size_t size, size_t* written);
MemoryRegions sys_memory_regions();