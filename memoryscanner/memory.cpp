#include "memory.h"


void merge_memory_regions(MemoryRegions& regions) {
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