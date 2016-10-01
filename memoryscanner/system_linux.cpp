#include "system_public.h"

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

struct LinuxVars {
	pid_t process_id;
	int process;
};

LinuxVars linux;

size_t sys_get_error() {
	return errno;
}

void sys_set_process_id(size_t process_id) {
	linux.process_id = static_cast<pid_t>(process_id);
}

bool sys_open_process(size_t process_id) {
	if(linux.process != 0 && linux.process != -1) {
		return false;
	}
	sys_set_process_id(process_id);
	char filename[64];
	sprintf(filename, "/proc/%d/mem", linux.process_id);
	linux.process = open(filename, O_RDWR);
	return linux.process != -1;
}

bool sys_close_process() {
	if(linux.process == 0 || linux.process == -1) {
		return false;
	}
	close(linux.process);
	linux.process = 0;
	return true;
}

bool sys_seek_memory(void* address) {
	return lseek(linux.process, reinterpret_cast<off_t>(address), SEEK_SET) != -1;
}

bool sys_read_memory(void* address, void* buffer, size_t size, size_t* read) {
	*read = ::read(linux.process, buffer, size);
	return *read != -1;
}

bool sys_write_memory(void* address, const void* buffer, size_t size, size_t* written) {
	*written = write(linux.process, buffer, size);
	return *written != -1;
}

MemoryRegions sys_memory_regions() {
	MemoryRegions memory_regions;

	char filename[64];
	sprintf(filename, "/proc/%d/maps", linux.process_id);
	FILE* file = fopen(filename, "r");
	if(file) {
		uint8_t* addr, *endaddr;
		long long offset, inode;
		char permissions[8], device[8], pathname[MAXPATHLEN];
		for(;;) {
			int ret = fscanf(file, "%p-%p %s %llx %s %llx%[^\n]", &addr, &endaddr, permissions, &offset, device, &inode, pathname);
			if(ret == 0 || ret == EOF) {
				break;
			}
			bool read = permissions[0] == 'r';
			bool write = permissions[1] == 'w';
			//bool exec = permissions[2] == 'x';
			//bool priv = permissions[3] == 'p';
			if(read && write) {
				memory_regions.emplace_back(addr, endaddr);
			}
		}
		fclose(file);
	}
	merge_memory_regions(memory_regions);
	return memory_regions;
}
