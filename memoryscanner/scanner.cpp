#include "scanner.h"
#include <iostream>
#include <algorithm>
#include <functional>

const size_t kBufferSize = 4096 * 32;

Scanner::Scanner(size_t process_id) : settings{4, ValueType::Int32, CompareType::Equal} {
	if(!sys_open_process(process_id)) {
		throw std::runtime_error("failed to open process");
	}
	results.reserve(10000000);
}

Scanner::~Scanner() {
	sys_close_process();
}

template<typename T, typename Compare = std::equal_to<>>
void find_in_buffer(uint8_t* begin, uint8_t* buffer_begin, uint8_t* buffer_end, int alignment, void* value, MemoryResults& results) {
	T val = *reinterpret_cast<T*>(value);
	for(uint8_t* i = buffer_begin; i < buffer_end; i += alignment) {
		if(Compare()(*reinterpret_cast<T*>(i), val)) {
			results.emplace_back(begin + (i - buffer_begin), *reinterpret_cast<T*>(i));
		}
	}
}

template<>
void find_in_buffer<std::string>(uint8_t* begin, uint8_t* buffer_begin, uint8_t* buffer_end, int alignment, void* value, MemoryResults& results) {
	std::string& val = *reinterpret_cast<std::string*>(value);
	auto data = val.data();
	size_t size = val.size();
	for(uint8_t* i = buffer_begin; i < buffer_end; i += alignment) {
		if(std::memcmp(i, data, size) == 0) {
			results.emplace_back(begin + (i - buffer_begin), 0);
		}
	}
}

void Scanner::find_first(int value) {
	std::function<void(uint8_t*, uint8_t*, uint8_t*, int, void*, MemoryResults&)> compare_method;
	compare_method = find_in_buffer<int, std::equal_to<>>;
	void* val = &value;
	for(const auto& region : sys_memory_regions()) {
		uint8_t* begin = region.begin;
		if(!sys_seek_memory(begin)) {
			std::cout << "Unable to seek memory location " << static_cast<void*>(begin) << ", error " << sys_get_error() << '\n';
			continue;
		}
		while(begin < region.end) {
			size_t read, bytesToRead = std::min(kBufferSize, static_cast<size_t>(region.end - begin));
			uint8_t buffer[kBufferSize];
			bool success = sys_read_memory(begin, &buffer, bytesToRead, &read);
			if(!success) {
				std::cout << "Failed to read value at " << static_cast<void*>(begin) << ", read " << read << '/' << bytesToRead << ", error " << sys_get_error() << '\n';
			}
			compare_method(begin, buffer, buffer + bytesToRead, settings.alignment, val, results);
			begin += kBufferSize;
		}
	}
}

void Scanner::find_next(int value) {
	auto result_it = results.begin();
	auto result_last = results.end();
	for(const auto& region : sys_memory_regions()) {
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
			size_t read, bytesToRead = std::min(kBufferSize, static_cast<size_t>(region.end - begin));
			uint8_t buffer[kBufferSize];
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