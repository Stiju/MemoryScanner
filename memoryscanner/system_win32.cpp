#include "system_public.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct Win32Vars {
	DWORD process_id;
	HANDLE process;
};

Win32Vars win32;

size_t sys_get_error() {
	return GetLastError();
}

void sys_set_process_id(size_t process_id) {
	win32.process_id = static_cast<DWORD>(process_id);
}

bool sys_open_process(size_t process_id) {
	if(win32.process) {
		return false;
	}
	sys_set_process_id(process_id);
	win32.process = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, false, win32.process_id);
	return win32.process != 0;
}

bool sys_close_process() {
	if(!win32.process) {
		return false;
	}
	::CloseHandle(win32.process);
	win32.process = nullptr;
	return true;
}

bool sys_seek_memory(void* address) { return true; }

bool sys_read_memory(void* address, void* buffer, size_t size, size_t* read) {
	return ReadProcessMemory(win32.process, address, buffer, size, reinterpret_cast<SIZE_T*>(read)) != 0;
}

bool sys_write_memory(void* address, const void* buffer, size_t size, size_t* written) {
	return WriteProcessMemory(win32.process, address, buffer, size, reinterpret_cast<SIZE_T*>(written)) != 0;
}

MemoryRegions sys_memory_regions() {
	MemoryRegions memory_regions;
	uint8_t* address = nullptr;

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	while(address < si.lpMaximumApplicationAddress) {
		MEMORY_BASIC_INFORMATION mbi;
		size_t bytes = VirtualQueryEx(win32.process, address, &mbi, sizeof(mbi));
		if(bytes == 0) {
			break;
		}
		if((mbi.Protect & PAGE_READWRITE) == PAGE_READWRITE && mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_NOACCESS) != PAGE_NOACCESS && (mbi.Protect & PAGE_GUARD) != PAGE_GUARD && (mbi.Protect & PAGE_NOCACHE) != PAGE_NOCACHE) {
			memory_regions.emplace_back(static_cast<uint8_t*>(mbi.BaseAddress), static_cast<uint8_t*>(mbi.BaseAddress) + mbi.RegionSize);
		}
		address += mbi.RegionSize;
	}
	return memory_regions;
}
