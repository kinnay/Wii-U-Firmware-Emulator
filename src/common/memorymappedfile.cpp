#include "memorymappedfile.h"
#include "exceptions.h"

#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#else
#include <Windows.h>
#endif

uint8_t *memory_mapped_file_open(const char *filename, size_t size) {
#ifndef _WIN32
	int fd = open(filename, O_RDWR);
	if (fd < 0) {
		runtime_error("Failed to open %s", filename);
	}
	void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (!ptr) {
		runtime_error("Failed to mmap %s", filename);
	}
	close(fd);
	return (uint8_t *)ptr;
#else
	HANDLE file = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, NULL);
	if (!file) {
		runtime_error("Failed to open file %s", filename);
	}
	HANDLE mapping = CreateFileMappingA(file, nullptr, PAGE_READWRITE, 0, 0, nullptr);
	if (!mapping) {
		runtime_error("Failed to create file mapping for %s", filename);
	}
	LPVOID ptr = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (!ptr) {
		runtime_error("Failed to map view of file for %s", filename);
	}
	CloseHandle(file);
	CloseHandle(mapping);
	return (uint8_t *)ptr;
#endif
}

void memory_mapped_file_close(uint8_t *addr, std::size_t size) {
#ifndef _WIN32
	munmap(addr, size);
#else
	UnmapViewOfFile(addr);
#endif
}
