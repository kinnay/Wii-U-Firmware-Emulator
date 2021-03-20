
#pragma once

#include "cpu/ppc/ppcinstruction.h"

#include <initializer_list>
#include <string>
#include <vector>
#include <map>

#include <cstdint>


class PPCGroup {
public:
	PPCGroup();
	PPCGroup(std::initializer_list<std::string> list);
	
	PPCGroup operator +(const PPCGroup &other);
	
	std::vector<std::string> instrs;
};


class PPCInstructions {
public:
	static PPCGroup Branch;
	static PPCGroup Condition;
	static PPCGroup LoadStore;
	static PPCGroup Integer;
	static PPCGroup FloatingPoint;
	static PPCGroup PairedSingles;
	static PPCGroup Caching;
	static PPCGroup System;
	
	static PPCGroup All;
};


class PPCMetrics {
public:
	enum PrintMode {
		CATEGORY,
		FREQUENCY
	};
	
	PPCMetrics();
	
	void reset();
	void print(PrintMode mode);
	void update(PPCInstruction instr);
	
private:
	const char *decode(PPCInstruction instr);
	uint64_t count(PPCGroup &group);
	
	std::map<std::string, uint64_t> instrs;
};
