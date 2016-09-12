#include "scanner.h"
#include <iostream>
#include <algorithm>

const int kBufferSize = 4096 * 32;

void merge_memory_blocks(MemoryRegions& regions) {
	for(auto it = regions.begin(); it != regions.end();) {
		auto first = it++;
		if(it == regions.end()) {
			break;
		}
		if(first->end == it->begin) {
			first->end = it->end;
			regions.erase(it);
			it = first;
		} else {
			++it;
		}
	}
}

Scanner::Scanner(size_t process_id) {
	if(!sys_open_process(process_id)) {
		throw std::runtime_error("failed to open process");
	}
	results.reserve(10000000);
}

Scanner::~Scanner() {
	sys_close_process();
}

void Scanner::find_first(int value) {
	regions = sys_memory_regions();
	std::cout << "Regions: " << regions.size() << '\n';
	merge_memory_blocks(regions);
	std::cout << "Regions: " << regions.size() << '\n';
	for(const auto& region : regions) {
		size_t read;
		size_t bytesToRead = (static_cast<uint8_t*>(region.end) - static_cast<uint8_t*>(region.begin));
		if(bytesToRead > kBufferSize) {
			bytesToRead = kBufferSize;
		}
		uint8_t* begin = static_cast<uint8_t*>(region.begin);
		int x = 0, hits = 0;
		if(!sys_seek_memory(begin)) {
			std::cout << "Unable to seek memory location " << static_cast<void*>(begin) << ", error " << sys_get_error() << '\n';
			continue;
		}
		while(begin < region.end) {
			uint8_t buffer[kBufferSize];
			bool success = sys_read_memory(begin, &buffer, bytesToRead, &read);
			if(!success) {
				std::cout << "Failed to read value at " << static_cast<void*>(begin) << ", read " << read << '/' << bytesToRead << ", error " << sys_get_error() << '\n';
			}
			for(uint8_t* i = buffer, *end = buffer + bytesToRead; i < end; i += sizeof(int)) {
				if(*reinterpret_cast<int*>(i) == value) {
					void* adr = begin + (i - buffer);
					results.emplace_back(adr, value);
					++hits;
				}
			}
			begin = begin + kBufferSize;
			if(begin < region.end && begin >(static_cast<uint8_t*>(region.end) - kBufferSize)) {
				bytesToRead = static_cast<uint8_t*>(region.end) - begin;
			}
			++x;
		}
	}
}

void Scanner::find_next(int value) {
	auto it = results.begin();
	for(const auto& region : regions) {
		auto end = std::lower_bound(it, results.end(), region.end,
			[](const MemoryResult& result, void* address) {return result.address < address; });
		if(it == end) {
			continue;
		}
		size_t read;
		size_t bytesToRead = static_cast<uint8_t*>((end - 1)->address) - static_cast<uint8_t*>(it->address) + sizeof(int);
		
		if(bytesToRead > kBufferSize) {
			bytesToRead = kBufferSize;
		}
		uint8_t* begin = static_cast<uint8_t*>(it->address);
		int x = 0, hits = 0;
		if(!sys_seek_memory(begin)) {
			std::cout << "Unable to seek memory location " << static_cast<void*>(begin) << ", error " << sys_get_error() << '\n';
			continue;
		}
		while(begin < region.end) {
			uint8_t buffer[kBufferSize];
			bool success = sys_read_memory(begin, &buffer, bytesToRead, &read);
			if(!success) {
				std::cout << "Failed to read value at " << static_cast<void*>(begin) << ", read " << read << '/' << bytesToRead << ", error " << sys_get_error() << '\n';
			}
			for(; it != end; ++it) {
				auto position = static_cast<uint8_t*>(it->address) - begin;
				if(position >= kBufferSize) {
					break;
				}
				it->value = *reinterpret_cast<int*>(buffer + position);
			}
			begin = begin + kBufferSize;
			if(begin < region.end && begin >(static_cast<uint8_t*>(region.end) - kBufferSize)) {
				bytesToRead = static_cast<uint8_t*>(region.end) - begin;
			}
			++x;
		}
	}
	results.erase(std::remove_if(results.begin(), results.end(), [value](auto& x) {return x.value != value; }), results.end());
}

void Scanner::find(int value) {
	if(results.size() == 0) {
		find_first(value);
	} else {
		find_next(value);
	}
}