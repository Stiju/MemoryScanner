#pragma once

#include "system_public.h"

class Scanner {
	MemoryRegions regions;
	MemoryResults results;
public:
	Scanner(size_t process_id);
	~Scanner();

	const MemoryResults& get_results() const {
		return results;
	}

	void clear() {
		results.clear();
	}

	void shrink() {
		results.shrink_to_fit();
	}

	size_t size() const {
		return results.size();
	}

	void find_first(int value);
	void find_next(int value);
	void find(int value);
};