#include "system_public.h"

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/vm_region.h>
#include <mach/vm_map.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <errno.h>

struct MacOsVars {
	pid_t process_id;
	mach_port_t process;
};

MacOsVars macos;

size_t sys_get_error() {
	return errno;
}

void sys_set_process_id(size_t process_id) {
	macos.process_id = static_cast<pid_t>(process_id);
}

bool sys_open_process(size_t process_id) {
	if(macos.process != 0) {
		return false;
	}
	sys_set_process_id(process_id);
	auto kret = task_for_pid(mach_task_self(), macos.process_id, &macos.process);
	return kret == KERN_SUCCESS;
}

bool sys_close_process() {
	if(macos.process == 0) {
		return false;
	}
	macos.process = 0;
	return true;
}

bool sys_seek_memory(void* /*address*/) { return true; }

bool sys_read_memory(void* address, void* buffer, size_t size, size_t* read) {
	auto kret = mach_vm_read_overwrite(macos.process, reinterpret_cast<mach_vm_address_t>(address), size, reinterpret_cast<mach_vm_address_t>(buffer), reinterpret_cast<mach_vm_size_t*>(read));
	return kret == KERN_SUCCESS;
}

bool sys_write_memory(void* address, const void* buffer, size_t size, size_t* written) {
	auto kret = mach_vm_write(macos.process, reinterpret_cast<mach_vm_address_t>(address), reinterpret_cast<vm_offset_t>(buffer), static_cast<mach_msg_type_number_t>(size));
	return kret == KERN_SUCCESS;
}

MemoryRegions sys_memory_regions() {
	MemoryRegions memory_regions;

	mach_vm_address_t address = 0;
	mach_vm_size_t size;
	mach_port_t object_name;
	mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
	for(;;) {
		vm_region_basic_info_data_t info;
		auto kret = mach_vm_region(macos.process, &address, &size, VM_REGION_BASIC_INFO, reinterpret_cast<vm_region_info_t>(&info), &count, &object_name);
		if(kret != KERN_SUCCESS) {
			break;
		}
		if((~info.protection & (VM_PROT_READ | VM_PROT_WRITE)) == 0) {
			memory_regions.emplace_back(reinterpret_cast<uint8_t*>(address), reinterpret_cast<uint8_t*>(address + size));
		}
		address += size;
	}

	//merge_memory_regions(memory_regions);
	return memory_regions;
}
