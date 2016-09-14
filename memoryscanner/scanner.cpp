#include "scanner.h"
#include <iostream>
#include <algorithm>

const long long kBufferSize = 4096 * 32;

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
					uint8_t* adr = begin + (i - buffer);
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
	auto result_it = results.begin();
	auto result_last = results.end();
	for(const auto& region : regions) {
		result_it = std::lower_bound(result_it, result_last, region.begin);
		if(result_it == result_last) { // reached the end
			break;
		}
		auto result_end = std::lower_bound(result_it, result_last, region.end);
		if(result_it == result_end) { // no results in current region
			continue;
		}

		uint8_t* begin = result_it->address;
		if(!sys_seek_memory(begin)) {
			std::cout << "Unable to seek memory location " << static_cast<void*>(begin) << ", error " << sys_get_error() << '\n';
			continue;
		}
		while(begin < region.end && result_it != result_end) {
			size_t read;
			uint8_t buffer[kBufferSize];
			size_t bytesToRead = std::min(kBufferSize, region.end - begin);
			if(!sys_read_memory(begin, &buffer, bytesToRead, &read)) {
				std::cout << "Failed to read value at " << static_cast<void*>(begin) << ", read " << read << '/' << bytesToRead << ", error " << sys_get_error() << '\n';
			}
			for(; result_it != result_end; ++result_it) {
				auto position = result_it->address - begin;
				if(position >= kBufferSize) {
					break;
				}
				result_it->value = *reinterpret_cast<int*>(buffer + position);
			}
			begin += kBufferSize;
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