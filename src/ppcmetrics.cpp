
#include "ppcmetrics.h"

#include "common/exceptions.h"
#include "common/sys.h"

#include <algorithm>


static uint64_t divide(uint64_t a, uint64_t b) {
	return b ? a / b : 0;
}

static int percentage(uint64_t value, uint64_t total) {
	return divide(value * 100, total);
}


PPCGroup::PPCGroup() {}

PPCGroup::PPCGroup(std::initializer_list<std::string> list) {
	for (std::string instr : list) {
		instrs.push_back(instr);
	}
}

PPCGroup PPCGroup::operator +(const PPCGroup &other) {
	PPCGroup group;
	for (std::string instr : instrs) {
		if (!instr.empty()) {
			group.instrs.push_back(instr);
		}
	}
	for (std::string instr : other.instrs) {
		if (!instr.empty()) {
			group.instrs.push_back(instr);
		}
	}
	return group;
}


PPCGroup PPCInstructions::Branch = {
	"b", "bc", "bclr", "bcctr"
};

PPCGroup PPCInstructions::Condition = {
	"mfcr", "mtcrf", "crxor"
};

PPCGroup PPCInstructions::LoadStore = {
	"lbz", "lbzx", "lbzu", "lbzux", "",
	"lhz", "lhzx", "lhzu", "lhzux", "",
	"lha", "lhax", "lhau", "lhaux", "",
	"lwz", "lwzx", "lwzu", "lwzux", "",
	"stb", "stbx", "stbu", "stbux", "",
	"sth", "sthx", "sthu", "sthux", "",
	"stw", "stwx", "stwu", "stwux", "",
	"lwbrx", "stwbrx", "",
	"lwarx", "stwcx", "",
	"lmw", "stmw"
};

PPCGroup PPCInstructions::Integer = {
	"add", "addc", "addic", "adde", "addze", "",
	"subf", "subfc", "subfic", "subfe", "subfze", "",
	"mulli", "mullw", "mulhw", "mulhwu", "",
	"divw", "divwu", "",
	"and", "andi", "andis", "andc", "",
	"or", "ori", "oris", "nor", "",
	"xor", "xori", "xoris", "",
	"slw", "srw", "sraw", "srawi", "",
	"rlwimi", "rlwinm", "",
	"cntlzw", "extsb", "extsh", "neg", "",
	"cmp", "cmpi", "cmpl", "cmpli"
};

PPCGroup PPCInstructions::FloatingPoint = {
	"lfs", "lfsx", "lfsu", "lfsux",
	"stfs", "stfsx", "stfsu", "stfsux", ""
	"lfd", "lfdx", "lfdu", "lfdux",
	"stfd", "stfdx", "stfdu", "stfdux", "",
	"fmr", "fneg", "fabs", "",
	"fadd", "fsub", "fmul", "fdiv",
	"fmadd", "fmsub", "fnmadd", "fnmsub", "",
	"fadds", "fsubs", "fmuls", "fdivs",
	"fmadds", "fmsubs", "fnmadds", "fnmsubs", "",
	"fsel", "frsp", "fctiwz", "stfiwx", "",
	"fcmpu", "mffs", "mtfsf", "",
	"frsqrte"
};

PPCGroup PPCInstructions::PairedSingles = {
	"psq_l", "psq_st", "",
	"ps_mr", "ps_neg", "ps_abs", "ps_nabs", "",
	"ps_add", "ps_sub", "ps_mul", "ps_div", "",
	"ps_madd", "ps_msub", "ps_nmadd", "ps_nmsub", "",
	"ps_sum0", "ps_sum1", "ps_muls0", "ps_muls1", "",
	"ps_merge00", "ps_merge01", "ps_merge10", "ps_merge11",
};

PPCGroup PPCInstructions::Caching = {
	"sync", "isync", "eieio", "tlbie", "",
	"icbi", "dcbf", "dcbst", "dcbi", "dcbz", "dcbz_l"
};

PPCGroup PPCInstructions::System = {
	"sc", "rfi", "",
	"mftb", "mfspr", "mtspr", "mfmsr", "mtmsr", "mfsr", "mtsr"
};

PPCGroup PPCInstructions::All =
	Branch + Condition + LoadStore + Integer +
	FloatingPoint + PairedSingles + Caching + System
;


bool compareFreq(std::pair<std::string, uint64_t> a, std::pair<std::string, uint64_t> b) {
	return a.second > b.second || (a.second == b.second && a.first.compare(b.first) < 0);
}


PPCMetrics::PPCMetrics() {
	reset();
}

void PPCMetrics::reset() {
	instrs.clear();
}

void PPCMetrics::print(PrintMode mode) {
	uint64_t total = count(PPCInstructions::All);
	
	Sys::stdout->write("Instructions executed: %i\n", total);
	Sys::stdout->write("\n");
	
	if (mode == CATEGORY) {
		uint64_t branch = count(PPCInstructions::Branch);
		uint64_t loadstore = count(PPCInstructions::LoadStore);
		uint64_t integer = count(PPCInstructions::Integer);
		uint64_t floatingpoint = count(PPCInstructions::FloatingPoint);
		uint64_t pairedsingles = count(PPCInstructions::PairedSingles);
		uint64_t condition = count(PPCInstructions::Condition);
		uint64_t caching = count(PPCInstructions::Caching);
		uint64_t system = count(PPCInstructions::System);
		
		Sys::stdout->write(
			"Branch instructions:   %10i (%i%%)\n",
			branch, percentage(branch, total)
		);
		Sys::stdout->write(
			"Load/store instrs:     %10i (%i%%)\n",
			loadstore, percentage(loadstore, total)
		);
		Sys::stdout->write(
			"Integer instructions:  %10i (%i%%)\n",
			integer, percentage(integer, total)
		);
		Sys::stdout->write(
			"Floating point instrs: %10i (%i%%)\n",
			floatingpoint, percentage(floatingpoint, total)
		);
		Sys::stdout->write(
			"Paired single instrs:  %10i (%i%%)\n",
			pairedsingles, percentage(pairedsingles, total)
		);
		Sys::stdout->write(
			"Condition register op: %10i (%i%%)\n",
			condition, percentage(condition, total)
		);
		Sys::stdout->write(
			"Cache/synchronization: %10i (%i%%)\n",
			caching, percentage(caching, total)
		);
		Sys::stdout->write(
			"System instructions:   %10i (%i%%)\n",
			system, percentage(system, total)
		);
		Sys::stdout->write("\n");
	}
	else {
		std::vector<std::pair<std::string, uint64_t>> list(instrs.begin(), instrs.end());
		std::sort(list.begin(), list.end(), compareFreq);
		
		Sys::stdout->write("Sorted by frequency:\n");
		for (std::pair<std::string, uint64_t> instr : list) {
			Sys::stdout->write(
				"    %-11s: %10i (%2i%%)\n", instr.first,
				instr.second, percentage(instr.second, total)
			);
		}
	}
}

uint64_t PPCMetrics::count(PPCGroup &group) {
	uint64_t total = 0;
	for (std::string instr : group.instrs) {
		total += instrs[instr];
	}
	return total;
}

void PPCMetrics::update(PPCInstruction instr) {
	const char *type = decode(instr);
	instrs[type]++;
}

const char *PPCMetrics::decode(PPCInstruction instr) {
	int type = instr.opcode();
	if (type == 4) {
		int type = instr.opcode2();
		if (type == 18) return "ps_div";
		else if (type == 20) return "ps_sub";
		else if (type == 21) return "ps_add";
		else if (type == 40) return "ps_neg";
		else if (type == 72) return "ps_mr";
		else if (type == 136) return "ps_nabs";
		else if (type == 264) return "ps_abs";
		else if (type == 528) return "ps_merge00";
		else if (type == 560) return "ps_merge01";
		else if (type == 592) return "ps_merge10";
		else if (type == 624) return "ps_merge11";
		else if (type == 1014) return "dcbz_l";
		else {
			int type = instr.opcode3();
			if (type == 10) return "ps_sum0";
			else if (type == 11) return "ps_sum1";
			else if (type == 12) return "ps_muls0";
			else if (type == 13) return "ps_muls1";
			else if (type == 25) return "ps_mul";
			else if (type == 28) return "ps_msub";
			else if (type == 29) return "ps_madd";
			else if (type == 30) return "ps_nmsub";
			else if (type == 31) return "ps_nmadd";
		}
	}
	else if (type == 7) return "mulli";
	else if (type == 8) return "subfic";
	else if (type == 10) return "cmpli";
	else if (type == 11) return "cmpi";
	else if (type == 12) return "addic";
	else if (type == 13) return "addic";
	else if (type == 14) return "addi";
	else if (type == 15) return "addis";
	else if (type == 16) return "bc";
	else if (type == 17) return "sc";
	else if (type == 18) return "b";
	else if (type == 19) {
		int type = instr.opcode2();
		if (type == 16) return "bclr";
		else if (type == 50) return "rfi";
		else if (type == 150) return "isync";
		else if (type == 193) return "crxor";
		else if (type == 528) return "bcctr";
	}
	else if (type == 20) return "rlwimi";
	else if (type == 21) return "rlwinm";
	else if (type == 24) return "ori";
	else if (type == 25) return "oris";
	else if (type == 26) return "xori";
	else if (type == 27) return "xoris";
	else if (type == 28) return "andi";
	else if (type == 29) return "andis";
	else if (type == 31) {
		int type = instr.opcode2();
		if (type == 0) return "cmp";
		else if (type == 8) return "subfc";
		else if (type == 10) return "addc";
		else if (type == 11) return "mulhwu";
		else if (type == 19) return "mfcr";
		else if (type == 20) return "lwarx";
		else if (type == 23) return "lwzx";
		else if (type == 24) return "slw";
		else if (type == 26) return "cntlzw";
		else if (type == 28) return "and";
		else if (type == 32) return "cmpl";
		else if (type == 40) return "subf";
		else if (type == 54) return "dcbst";
		else if (type == 55) return "lwzux";
		else if (type == 60) return "andc";
		else if (type == 75) return "mulhw";
		else if (type == 83) return "mfmsr";
		else if (type == 86) return "dcbf";
		else if (type == 87) return "lbzx";
		else if (type == 104) return "neg";
		else if (type == 119) return "lbzux";
		else if (type == 124) return "nor";
		else if (type == 136) return "subfe";
		else if (type == 138) return "adde";
		else if (type == 144) return "mtcrf";
		else if (type == 146) return "mtmsr";
		else if (type == 150) return "stwcx";
		else if (type == 151) return "stwx";
		else if (type == 183) return "stwux";
		else if (type == 200) return "subfze";
		else if (type == 202) return "addze";
		else if (type == 210) return "mtsr";
		else if (type == 215) return "stbx";
		else if (type == 235) return "mullw";
		else if (type == 247) return "stbux";
		else if (type == 266) return "add";
		else if (type == 279) return "lhzx";
		else if (type == 306) return "tlbie";
		else if (type == 311) return "lhzux";
		else if (type == 316) return "xor";
		else if (type == 339) return "mfspr";
		else if (type == 343) return "lhax";
		else if (type == 371) return "mftb";
		else if (type == 375) return "lhaux";
		else if (type == 407) return "sthx";
		else if (type == 439) return "sthux";
		else if (type == 444) return "or";
		else if (type == 459) return "divwu";
		else if (type == 467) return "mtspr";
		else if (type == 470) return "dcbi";
		else if (type == 491) return "divw";
		else if (type == 534) return "lwbrx";
		else if (type == 535) return "lfsx";
		else if (type == 536) return "srw";
		else if (type == 567) return "lfsux";
		else if (type == 595) return "mfsr";
		else if (type == 598) return "sync";
		else if (type == 599) return "lfdx";
		else if (type == 631) return "lfdux";
		else if (type == 662) return "stwbrx";
		else if (type == 663) return "stfsx";
		else if (type == 695) return "stfsux";
		else if (type == 727) return "stfdx";
		else if (type == 759) return "stfdux";
		else if (type == 792) return "sraw";
		else if (type == 824) return "srawi";
		else if (type == 854) return "eieio";
		else if (type == 922) return "extsh";
		else if (type == 954) return "extsb";
		else if (type == 982) return "icbi";
		else if (type == 983) return "stfiwx";
		else if (type == 1014) return "dcbz";
	}
	else if (type == 32) return "lwz";
	else if (type == 33) return "lwzu";
	else if (type == 34) return "lbz";
	else if (type == 35) return "lbzu";
	else if (type == 36) return "stw";
	else if (type == 37) return "stwu";
	else if (type == 38) return "stb";
	else if (type == 39) return "stbu";
	else if (type == 40) return "lhz";
	else if (type == 41) return "lhzu";
	else if (type == 42) return "lha";
	else if (type == 43) return "lhau";
	else if (type == 44) return "sth";
	else if (type == 45) return "sthu";
	else if (type == 46) return "lmw";
	else if (type == 47) return "stmw";
	else if (type == 48) return "lfs";
	else if (type == 49) return "lfsu";
	else if (type == 50) return "lfd";
	else if (type == 51) return "lfdu";
	else if (type == 52) return "stfs";
	else if (type == 53) return "stfsu";
	else if (type == 54) return "stfd";
	else if (type == 55) return "stfdu";
	else if (type == 56) return "psq_l";
	else if (type == 59) {
		int type = instr.opcode3();
		if (type == 18) return "fdivs";
		else if (type == 20) return "fsubs";
		else if (type == 21) return "fadds";
		else if (type == 25) return "fmuls";
		else if (type == 28) return "fmsubs";
		else if (type == 29) return "fmadds";
		else if (type == 30) return "fnmsubs";
		else if (type == 31) return "fnmadds";
	}
	else if (type == 60) return "psq_st";
	else if (type == 63) {
		int type = instr.opcode2();
		if (type == 0) return "fcmpu";
		else if (type == 12) return "frsp";
		else if (type == 15) return "fctiwz";
		else if (type == 18) return "fdiv";
		else if (type == 20) return "fsub";
		else if (type == 21) return "fadd";
		else if (type == 26) return "frsqrte";
		else if (type == 40) return "fneg";
		else if (type == 72) return "fmr";
		else if (type == 264) return "fabs";
		else if (type == 583) return "mffs";
		else if (type == 711) return "mtfsf";
		else {
			int type = instr.opcode3();
			if (type == 23) return "fsel";
			else if (type == 25) return "fmul";
			else if (type == 28) return "fmsub";
			else if (type == 29) return "fmadd";
			else if (type == 30) return "fnmsub";
			else if (type == 31) return "fnmadd";
		}
	}
	runtime_error("Unknown PPC instruction: 0x%08X", instr.value);
	return "";
}
