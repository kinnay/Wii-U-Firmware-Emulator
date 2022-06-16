
#include "hardware/aes.h"

#include "physicalmemory.h"

#include "common/logger.h"
#include "common/endian.h"

#include <cstring>


AESController::AESController(PhysicalMemory *physmem) {
	this->physmem = physmem;
}

void AESController::reset() {
	ctrl = 0;
	src = 0;
	dest = 0;
	
	memset(key, 0, sizeof(key));
	memset(iv, 0, sizeof(iv));
	
	interrupt = false;
}

bool AESController::check_interrupts() {
	bool intr = interrupt;
	interrupt = false;
	return intr;
}

uint32_t AESController::read(uint32_t addr) {
	switch (addr) {
		case AES_CTRL: return ctrl;
	}
	
	Logger::warning("Unknown aes read: 0x%X", addr);
	return 0;
}

void AESController::write(uint32_t addr, uint32_t value) {
	if (addr == AES_CTRL) {
		ctrl = value;
		if (value >> 31) {
			ctrl = (ctrl & ~0x80000000) | 0xFFF;
			
			size_t size = ((value & 0xFFF) + 1) * 16;
			
			Buffer data = physmem->read(src, size);
			
			if (value & 0x10000000) {
				if (!(value & 0x1000)) {
					if (value & 0x8000000) AES_set_decrypt_key((uint8_t *)key, 128, &aeskey);
					else {
						AES_set_encrypt_key((uint8_t *)key, 128, &aeskey);
					}

					memcpy(aesiv, iv, 16);
				}
				
				if (value & 0x8000000) {
					AES_cbc_encrypt(data.get(), data.get(), size, &aeskey, aesiv, AES_DECRYPT);
				}
				else {
					AES_cbc_encrypt(data.get(), data.get(), size, &aeskey, aesiv, AES_ENCRYPT);
				}
			}
			
			physmem->write(dest, data);
			
			if (value & 0x40000000) {
				interrupt = true;
			}
		}
	}
	else if (addr == AES_SRC) src = value;
	else if (addr == AES_DEST) dest = value;
	else if (addr == AES_KEY) {
		for (int i = 0; i < 3; i++) {
			key[i] = key[i + 1];
		}
		key[3] = Endian::swap32(value);
	}
	else if (addr == AES_IV) {
		for (int i = 0; i < 3; i++) {
			iv[i] = iv[i + 1];
		}
		iv[3] = Endian::swap32(value);
	}
	else {
		Logger::warning("Unknown aes write: 0x%X (0x%08X)", addr, value);
	}
}