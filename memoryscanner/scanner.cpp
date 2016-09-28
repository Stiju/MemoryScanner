#include "scanner.h"
#include <iostream>
#include <algorithm>
#include <functional>
#include <cstring>

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
			results.emplace_back(begin + (i - buffer_begin), *reinterpret_cast<int*>(i));
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

struct UnknownValue {
	template<typename T>
	constexpr bool operator()(const T&, const T&) const {
		return true;
	}
};

template<typename T>
auto get_compare_method(CompareType compare_type) {
	switch(compare_type) {
	case CompareType::Equal: return find_in_buffer<T, std::equal_to<>>;
	case CompareType::Less: return find_in_buffer<T, std::less<>>;
	case CompareType::Greater: return find_in_buffer<T, std::greater<>>;
	case CompareType::Unknown:
	default: return find_in_buffer<int, UnknownValue>;
	}
}

void Scanner::find_first(const std::string& value) {
	std::function<void(uint8_t*, uint8_t*, uint8_t*, int, void*, MemoryResults&)> compare_method;
	std::cout << sizeof(compare_method) << '\n';
	union DataValue {
		int8_t int8;
		int16_t int16;
		int32_t int32;
		int64_t int64;
		float float_value;
		double double_value;
	} dv;
	void* val = &dv;
	switch(settings.value_type) {
	case ValueType::Int8:
		compare_method = get_compare_method<int8_t>(settings.compare_type);
		dv.int8 = static_cast<int8_t>(std::stoi(value));
		break;
	case ValueType::Int16:
		compare_method = get_compare_method<int16_t>(settings.compare_type);
		dv.int16 = static_cast<int16_t>(std::stoi(value));
		break;
	case ValueType::Int32:
		compare_method = get_compare_method<int32_t>(settings.compare_type);
		dv.int32 = static_cast<int32_t>(std::stoi(value));
		break;
	case ValueType::Int64:
		compare_method = get_compare_method<int64_t>(settings.compare_type);
		dv.int64 = static_cast<int64_t>(std::stoll(value));
		break;
	case ValueType::Float:
		compare_method = get_compare_method<float>(settings.compare_type);
		dv.float_value = std::stof(value);
		break;
	case ValueType::Double:
		compare_method = get_compare_method<double>(settings.compare_type);
		dv.double_value = std::stod(value);
		break;
	case ValueType::String:
		compare_method = find_in_buffer<std::string, std::equal_to<>>;
		val = &const_cast<std::string&>(value);
		break;
	}
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

void Scanner::find_next(const std::string& value) {
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
	//results.erase(std::remove_if(results.begin(), results.end(), [value](auto& x) {return x.value != value; }), results.end());
}

void Scanner::find(const std::string& value) {
	if(results.size() == 0) {
		find_first(value);
	} else {
		find_next(value);
	}
}