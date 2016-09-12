#include <iostream>
#include <algorithm>
#include <exception>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

#include "system_public.h"
#include "stopwatch.h"

std::string GetInput() {
	std::string input;
	std::getline(std::cin, input);
	return input;
}
std::string GetInputLower() {
	std::string input;
	std::getline(std::cin, input);
	std::transform(input.begin(), input.end(), input.begin(), ::tolower);
	return input;
}
int GetInputInt() {
	for(;;) {
		try {
			auto input = GetInput();
			return std::stoi(input);
		} catch(const std::logic_error& ex) {
			std::cout << ex.what() << '\n';
		}
	}
}

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

const int kBufferSize = 4096 * 32;

class Scanner {
	MemoryRegions regions;
	MemoryResults results;
public:
	Scanner(size_t process_id) {
		if(!sys_open_process(process_id)) {
			throw std::runtime_error("failed to open process");
		}
	}
	~Scanner() {
		sys_close_process();
	}

	const MemoryResults& get_results() const {
		return results;
	}

	void clear() {
		results.clear();
	}

	size_t size() const {
		return results.size();
	}

	void find_first(int value) {
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
					if(*(int*)i == value) {
						void* adr = begin + (i - buffer);
						results.emplace_back(adr, value);
						++hits;
					}
				}
				begin = begin + kBufferSize;
				if(begin < region.end && begin > (static_cast<uint8_t*>(region.end) - kBufferSize)) {
					bytesToRead = static_cast<uint8_t*>(region.end) - begin;
				}
				++x;
			}
		}
	}

	void find_next(int value) {
		for(auto& result : results) {
			size_t read;
			if(!sys_seek_memory(result.address)) {
				std::cout << "Unable to seek memory location, error " << sys_get_error() << '\n';
				continue;
			}
			bool success = sys_read_memory(result.address, &result.value, 4, &read);
			if(!success) {
				std::cout << "Failed to read value at " << result.address << ", read " << read << '/' << sizeof(int) << ", error " << sys_get_error() << '\n';
			}
		}
		results.erase(std::remove_if(results.begin(), results.end(), [value](auto& x) {return x.value != value; }), results.end());
	}

	void find(int value) {
		if(results.size() == 0) {
			find_first(value);
		} else {
			find_next(value);
		}
	}
};

int main() {
	size_t pid;
	std::cout << "Enter process id: ";
	pid = GetInputInt();

	Scanner scanner{pid};
	for(;;) {
		if(scanner.size() == 0) {
			std::cout << ":: New Scan\n";
		} else {
			std::cout << ":: Next Scan\n";
		}
		std::cout << "Search for: ";
		int value = GetInputInt();
		if(value == -1) {
			break;
		} else if(value == -2) {
			std::cout << "Clearing results\n";
			scanner.clear();
			continue;
		}

		scanner.find(value);

		auto results = scanner.get_results();
		std::cout << "results: " << results.size() << '\n';
		int i = 0;
		for(const auto& adr : results) {
			std::cout << adr.address << '\n';
			if(++i == 10) {
				break;
			}
		}
	}
}
