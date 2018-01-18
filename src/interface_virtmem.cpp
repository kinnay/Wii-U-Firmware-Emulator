
#include "interface_virtmem.h"

IVirtualMemory::IVirtualMemory() :
	enabled(false), dataTranslation(false), instrTranslation(false), supervisorMode(true), cacheEnabled(false) {}

void IVirtualMemory::setEnabled(bool enabled) { this->enabled = enabled; }
void IVirtualMemory::setDataTranslation(bool enabled) { dataTranslation = enabled; }
void IVirtualMemory::setInstructionTranslation(bool enabled) { instrTranslation = enabled; }
void IVirtualMemory::setSupervisorMode(bool enabled) {
	supervisorMode = enabled;
	cache.invalidate();
}
void IVirtualMemory::setCacheEnabled(bool enabled) { cacheEnabled = enabled; }
void IVirtualMemory::invalidateCache() { cache.invalidate(); }
