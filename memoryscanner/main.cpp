#include <iostream>
#include <algorithm>
#include <exception>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

#include "system_public.h"
#include "scanner.h"
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

		{
			StopWatch sw;
			scanner.find(value);
		}
		scanner.shrink();

		auto& results = scanner.get_results();
		std::cout << "results: " << results.size() << '\n';
		int i = 0;
		for(const auto& adr : results) {
			std::cout << static_cast<void*>(adr.address) << " : " << adr.value << '\n';
			if(++i == 10) {
				break;
			}
		}
	}
}
