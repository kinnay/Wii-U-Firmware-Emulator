
#pragma once

#include "cpu/processor.h"

#include "physicalmemory.h"
#include "config.h"

#include <sys/mman.h>

#include <cstring>
#include <cstdlib>
#include <cstdint>


template <class TGenerator, class TValue>
class JIT {
public:
	typedef void *(*JITEntryFunc)(Processor *cpu);
	
	JIT(PhysicalMemory *physmem, Processor *cpu) {
		this->physmem = physmem;
		this->cpu = cpu;
		
		memset(table, 0, sizeof(table));
		memset(sizes, 0, sizeof(sizes));
	}
	
	void reset() {
		invalidate();
		
		#if STATS
		instrsCompiled = 0;
		instrsExecuted = 0;
		#endif
	}
	
	void invalidateBlock(uint32_t addr) {
		int index = addr >> 12;
		if (table[index]) {
			munmap(table[index], sizes[index]);
			table[index] = nullptr;
			
			#if STATS
			instrSize -= sizes[index];
			#endif
		}
	}
	
	void invalidate() {
		for (int i = 0; i < 0x100000; i++) {
			if (table[i]) {
				munmap(table[i], sizes[i]);
				table[i] = nullptr;
			}
		}
		
		#if STATS
		instrSize = 0;
		#endif
	}
	
	void execute(uint32_t pc) {
		char *target = table[pc >> 12];
		if (!target) {
			target = generateCode(pc & ~0xFFF);
			table[pc >> 12] = target;
		}
		
		target = target + 5 * ((pc & 0xFFF) / sizeof(TValue));
		
		#if STATS
		instrsExecuted++;
		#endif
		
		((JITEntryFunc)target)(cpu);
	}
	
	#if STATS
	uint64_t instrsExecuted;
	uint64_t instrsCompiled;
	uint64_t instrSize;
	#endif
	
private:
	char *generateCode(uint32_t pc) {
		TGenerator generator;
		for (int offs = 0; offs < 0x1000; offs += sizeof(TValue)) {
			TValue value = physmem->read<TValue>(pc + offs);
			generator.generate(value);
		}
		
		#if STATS
		instrsCompiled += 0x1000 / sizeof(TValue);
		instrSize += generator.size();
		#endif
		
		char *buffer = generator.get();
		size_t size = generator.size();
		
		sizes[pc >> 12] = size;
		
		char *jit = (char *)mmap(
			NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_ANONYMOUS | MAP_SHARED, -1, 0
		);
		memcpy(jit, buffer, size);
		return jit;
	}
	
	PhysicalMemory *physmem;
	Processor *cpu;
	
	char *table[0x100000];
	uint32_t sizes[0x100000];
};
