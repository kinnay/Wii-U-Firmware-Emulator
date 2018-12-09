
#include "class_elffile.h"
#include "class_filemgr.h"
#include "class_endian.h"

void ELFHeader::swapEndian() {
	Endian::swap((uint16_t *)&type);
	Endian::swap(&version2);
	Endian::swap(&entry);
	Endian::swap(&programOffs);
	Endian::swap(&sectionOffs);
	Endian::swap(&flags);
	Endian::swap(&headerSize);
	Endian::swap(&programSize);
	Endian::swap(&programNum);
	Endian::swap(&sectionSize);
	Endian::swap(&sectionNum);
	Endian::swap(&stringSection);
}

void ELFProgram::swapEndian() {
	Endian::swap((uint32_t *)&type);
	Endian::swap(&fileoffs);
	Endian::swap(&vaddr);
	Endian::swap(&paddr);
	Endian::swap(&filesize);
	Endian::swap(&memsize);
	Endian::swap(&flags);
	Endian::swap(&alignment);
}

void ELFSection::swapEndian() {
	Endian::swap(&nameoffs);
	Endian::swap((uint32_t *)&type);
	Endian::swap(&flags);
	Endian::swap(&addr);
	Endian::swap(&offset);
	Endian::swap(&size);
	Endian::swap(&link);
	Endian::swap(&info);
	Endian::swap(&alignment);
	Endian::swap(&entrySize);
}

ELFFile::~ELFFile() {
	if (buffer) {
		delete buffer;
	}
}

bool ELFFile::init(const void *data, uint32_t length) {
	buffer = new Buffer();
	if (!buffer) {
		MemoryError("Couldn't allocate buffer object");
		return false;
	}
	
	if (!buffer->init(data, length)) return false;
	
	header = buffer->ptr<ELFHeader>(0);
	if (!header) return false;

	bool swapEndian = (header->endianness == ELFHeader::Big) != (Endian::getSystemEndian() == Endian::Big);

	if (swapEndian) header->swapEndian();
	
	if (
		header->magic[0] != 0x7F || header->magic[1] != 'E' ||
		header->magic[2] != 'L' || header->magic[3] != 'F'
	) {
		ParseError("Invalid file identifier");
		return false;
	}
	
	if (header->bitsize != ELFHeader::X32) {
		NotImplementedError("Only 32-bit ELF files are supported");
		return false;
	}
	
	if (header->version1 != 1 || header->version2 != 1) {
		ParseError("Invalid ELF version");
		return false;
	}
	
	if (header->headerSize != sizeof(ELFHeader)) {
		ParseError("header->headerSize doesn't match sizeof(ELFHeader)");
		return false;
	}
	
	if (header->programNum && header->programSize != sizeof(ELFProgram)) {
		ParseError("header->programSize doesn't match sizeof(ELFProgram)");
		return false;
	}
	
	if (header->sectionNum && header->sectionSize != sizeof(ELFSection)) {
		ParseError("header->sectionSize doesn't match sizeof(ELFSection)");
		return false;
	}
	
	for (int i = 0; i < header->programNum; i++) {
		uint32_t offs = header->programOffs + header->programSize * i;
		ELFProgram *program = buffer->ptr<ELFProgram>(offs);
		if (!program) return false;

		if (swapEndian) program->swapEndian();
		
		programs.push_back(program);
	}
	
	for (int i = 0; i < header->sectionNum; i++) {
		uint32_t offs = header->sectionOffs + header->sectionSize * i;
		ELFSection *section = buffer->ptr<ELFSection>(offs);
		if (!section) return false;
		
		if (swapEndian) section->swapEndian();
		
		sections.push_back(section);
	}
	
	return true;
}
