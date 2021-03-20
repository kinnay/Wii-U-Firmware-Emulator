
#pragma once

#include "common/refcountedobj.h"
#include <vector>
#include <cstdint>

class MemoryRegion : public RefCountedObj {
public:
	enum Access {
		NoAccess, ReadOnly, ReadWrite
	};
	
	MemoryRegion();
	
	bool check();
	bool overlaps(Ref<MemoryRegion> region);
	bool hits(Ref<MemoryRegion> region);
	void merge(Ref<MemoryRegion> region);
	
	static bool compare(Ref<MemoryRegion> a, Ref<MemoryRegion> b);
	
	uint64_t virt;
	uint64_t phys;
	uint32_t size;
	Access user;
	Access system;
	bool executable;
};

class MemoryMap {
public:
	MemoryMap(bool nx);
	
	void add(Ref<MemoryRegion> region);
	void print();
	
private:
	Ref<MemoryRegion> find(Ref<MemoryRegion> region);
	
	std::vector<Ref<MemoryRegion>> regions;
	bool nx;
};
