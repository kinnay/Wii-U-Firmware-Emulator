
#pragma once

#include "class_mmucache.h"
#include <cstdint>

class IVirtualMemory {
public:
	enum Access {
		Instruction,
		DataRead,
		DataWrite
	};
	
	IVirtualMemory();
	
	virtual bool translate(uint32_t *addr, uint32_t length, Access type) = 0;
	void setEnabled(bool enabled);
	void setDataTranslation(bool enabled);
	void setInstructionTranslation(bool enabled);
	void setSupervisorMode(bool enabled);
	
	void setCacheEnabled(bool enabled);
	void invalidateCache();
	
protected:
	bool enabled;
	bool dataTranslation;
	bool instrTranslation;
	bool supervisorMode;
	
	MMUCache cache;
	bool cacheEnabled;
};
