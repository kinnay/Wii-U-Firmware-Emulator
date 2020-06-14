
#pragma once
#include <cstddef>
#include <cstdint>

uint8_t *memory_mapped_file_open(const char *filename, size_t size);
void memory_mapped_file_close(uint8_t *addr, size_t size);
