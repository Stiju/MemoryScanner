#pragma once

#include "system_public.h"

enum class ValueType {
	Int8,
	Int16,
	Int32,
	Int64,
	Float,
	Double,
	String
};

enum class CompareType {
	Equal,
	Less,
	Greater,
	Unknown
};

class Scanner {
	MemoryResults results;
	struct Settings {
		int alignment;
		ValueType value_type;
		CompareType compare_type;
	} settings;
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