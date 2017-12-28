
#pragma once

#include "class_buffer.h"
#include "errors.h"

#include <vector>
#include <cstdint>

struct ELFHeader {
	enum Class : uint8_t {
		X32 = 1,
		X64
	};
	
	enum Endianness : uint8_t {
		Little = 1,
		Big
	};
	
	enum Type : uint16_t {
		Relocatable = 1,
		Executable,
		Shared,
		Core
	};

	char magic[4];
	Class bitsize;
	Endianness endianness;
	uint8_t version1;
	uint8_t osType;
	uint8_t osVersion;
	char pad[7];
	
	Type type;
	uint16_t machine;
	uint32_t version2;
	uint32_t entry;
	
	uint32_t programOffs;
	uint32_t sectionOffs;
	uint32_t flags;
	uint16_t headerSize;
	uint16_t programSize;
	uint16_t programNum;
	uint16_t sectionSize;
	uint16_t sectionNum;
	uint16_t stringSection;
	
	void swapEndian();
};

struct ELFProgram {
	enum Type : uint32_t {
		PT_NULL,
		PT_LOAD,
		PT_DYNAMIC,
		PT_INTERP,
		PT_NOTE,
		PT_SHLIB,
		PT_PHDR
	};
	
	Type type;
	uint32_t fileoffs;
	uint32_t vaddr;
	uint32_t paddr;
	uint32_t filesize;
	uint32_t memsize;
	uint32_t flags;
	uint32_t alignment;
	
	void swapEndian();
};

struct ELFSection {
	enum Type : uint32_t {
		SHT_NULL,
		SHT_PROGBITS,
		SHT_SYMTAB,
		SHT_STRTAB,
		SHT_RELA,
		SHT_HASH,
		SHT_DYNAMIC,
		SHT_NOTE,
		SHT_NOBITS,
		SHT_REL,
		SHT_SHLIB,
		SHT_DYNSYM,
		SHT_INIT_ARRAY,
		SHT_FINI_ARRAY,
		SHT_PREINIT_ARRAY,
		SHT_GROUP,
		SHT_SYMTAB_SHNDX,
		SHT_NUM
	};
	
	uint32_t nameoffs;
	Type type;
	uint32_t flags;
	uint32_t addr;
	uint32_t offset;
	uint32_t size;
	uint32_t link;
	uint32_t info;
	uint32_t alignment;
	uint32_t entrySize;
	
	void swapEndian();
};

class ELFFile {
	public:
	~ELFFile();
	bool init(const void *data, uint32_t length);
	
	ELFHeader *header;
	std::vector<ELFProgram *> programs;
	std::vector<ELFSection *> sections;
	
	private:
	Buffer *buffer;
};
