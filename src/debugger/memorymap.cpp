
#include "debugger/memorymap.h"

#include "common/sys.h"

#include <algorithm>


const char *AccessProtNames[] = {
	" no access", " read only", "read/write"
};


MemoryRegion::MemoryRegion() {
	virt = 0;
	phys = 0;
	size = 0;
	user = NoAccess;
	system = NoAccess;
	executable = true;
}

bool MemoryRegion::check() {
	if (size == 0) return false;
	if (virt + size > 0x100000000) return false;
	if (phys + size > 0x100000000) return false;
	return true;
}

bool MemoryRegion::overlaps(Ref<MemoryRegion> other) {
	return virt < other->virt + other->size && virt + size > other->virt;
}

bool MemoryRegion::hits(Ref<MemoryRegion> other) {
	if (user != other->user) return false;
	if (system != other->system) return false;
	return virt + size == other->virt || virt == other->virt + other->size;
}

void MemoryRegion::merge(Ref<MemoryRegion> other) {
	if (other->virt < virt) {
		virt = other->virt;
	}
	size += other->size;
}

bool MemoryRegion::compare(Ref<MemoryRegion> a, Ref<MemoryRegion> b) {
	return a->virt < b->virt;
}


MemoryMap::MemoryMap(bool nx) {
	this->nx = nx;
}

Ref<MemoryRegion> MemoryMap::find(Ref<MemoryRegion> region) {
	for (Ref<MemoryRegion> other : regions) {
		if (other->overlaps(region)) {
			runtime_error("Memory regions overlap");
		}
		if (other->hits(region)) {
			return other;
		}
	}
	return nullptr;
}

void MemoryMap::add(Ref<MemoryRegion> region) {
	if (!region->check()) runtime_error("Memory region is invalid");
	
	Ref<MemoryRegion> prev = find(region);
	if (prev) {
		prev->merge(region);
	}
	else {
		regions.push_back(region);
	}
}

void MemoryMap::print() {
	if (regions.size() == 0) {
		Sys::out->write("    no pages mapped\n");
		return;
	}
	
	std::sort(regions.begin(), regions.end(), MemoryRegion::compare);
	
	for (Ref<MemoryRegion> region : regions) {
		Sys::out->write(
			"    %08X-%08X => %08X-%08X (user: %s, system: %s",
			region->virt, region->virt + region->size,
			region->phys, region->phys + region->size,
			AccessProtNames[region->user],
			AccessProtNames[region->system]
		);
		
		if (nx) {
			Sys::out->write(", %s", region->executable ? "executable" : "no execute");
		}
		Sys::out->write(")\n");
	}
}
