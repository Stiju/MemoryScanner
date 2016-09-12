#pragma once
#include <iostream>
#include <chrono>
#include <string>

class StopWatch {
	std::string name;
	std::chrono::high_resolution_clock::time_point start;
public:
	StopWatch(std::string&& name = "StopWatch") : name(std::move(name)), start(std::chrono::high_resolution_clock::now()) {}
	~StopWatch() {
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = end - start;
		if(duration > std::chrono::seconds(1)) {
			std::cout << name << ": " << std::chrono::duration_cast<std::chrono::duration<double>>(duration).count() << " s\n";
		} else if(duration > std::chrono::milliseconds(1)) {
			std::cout << name << ": " << std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(duration).count() << " ms\n";
		} else if(duration > std::chrono::microseconds(1)) {
			std::cout << name << ": " << std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(duration).count() << " us\n";
		} else {
			std::cout << name << ": " << std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() << " ns\n";
		}
	}
};
