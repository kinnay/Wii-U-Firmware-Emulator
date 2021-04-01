
#pragma once

#include "cpu/processor.h"
#include "common/bits.h"
#include "logger.h"
#include <vector>
#include <cstdint>


class DSPStack {
public:
	DSPStack(int capacity);
	
	void clear();
	void push(uint16_t value);
	uint16_t peek();
	uint16_t pop();
	
	uint16_t get(int index);
	bool empty();
	int size();

private:
	int capacity;
	std::vector<uint16_t> stack;
};


class DSPInterpreter : public Processor {
public:
	union Acc40 {
		struct {
			uint16_t l;
			uint16_t m;
			uint8_t h;
		};
		uint64_t value;
	};
	
	union Acc32 {
		struct {
			uint16_t l;
			uint16_t h;
		};
		uint32_t value;
	};
	
	enum Status {
		C = 1 << 0,
		O = 1 << 1,
		Z = 1 << 2,
		S = 1 << 3,
		LZ = 1 << 6,
		AM = 1 << 13,
		SXM = 1 << 14,
		SU = 1 << 15
	};
	
	// The Wii U model has many new registers.
	enum Memory {
		DMA_ADDR_MAIN_H = 0xFFBA,
		
		DRC_AI_BUFF_M = 0xFFBE,
		DRC_AI_BUFF_L = 0xFFBF,
		
		DSCR = 0xFFC9,
		
		DRC_AI_BUFF_H = 0xFFCA,
		
		DSBL = 0xFFCB,
		DSPA = 0xFFCD,
		DSMAH = 0xFFCE,
		DSMAL = 0xFFCF,
		
		TV_AI_BUFF_L = 0xFFD0,
		
		FFD2 = 0xFFD2,
		
		TV_AI_BUFF_H = 0xFFEE,
		TV_AI_BUFF_M = 0xFFFA,
		
		DIRQ = 0xFFFB,
		DMBH = 0xFFFC,
		DMBL = 0xFFFD,
		CMBH = 0xFFFE,
		CMBL = 0xFFFF
	};
	
	enum Register {
		AR0 = 0,
		AR1 = 1,
		AR2 = 2,
		AR3 = 3,
		
		IX0 = 4,
		IX1 = 5,
		IX2 = 6,
		IX3 = 7,
		
		WR0 = 8,
		WR1 = 9,
		WR2 = 10,
		WR3 = 11,
		
		ST0 = 12,
		ST1 = 13,
		ST2 = 14,
		ST3 = 15,
		
		AC0H = 16,
		AC1H = 17,
		
		CONFIG = 18,
		
		SR = 19,
		
		PRODL = 20,
		PRODM1 = 21,
		PRODH = 22,
		PRODM2 = 23,
		
		AX0L = 24,
		AX1L = 25,
		AX0H = 26,
		AX1H = 27,
		
		AC0L = 28,
		AC1L = 29,
		AC0M = 30,
		AC1M = 31
	};
	
	DSPInterpreter(Emulator *emulator);
	
	void reset();
	void step();

	uint16_t readreg(int reg);

	bool irq;
	
	int timer;

	uint16_t mailbox_in_h;
	uint16_t mailbox_in_l;
	uint16_t mailbox_out_h;
	uint16_t mailbox_out_l;
	
	uint32_t dma_addr_main;
	uint16_t dma_addr_dsp;
	uint16_t dma_control;
	
	uint16_t ffd2;
	
	uint16_t pc;
	
	uint16_t ar[4];
	uint16_t ix[4];
	uint16_t wr[4];
	DSPStack st[4];
	
	Acc40 ac[2];
	Acc32 ax[2];
	
	uint16_t config;
	
	Bits<uint16_t> status;

	uint16_t iram[0x2000];
	uint16_t irom[0x1000];
	
	// The Wii U model increased the amount of DRAM
	// compared to the Wii. DRAM now has 0x3000 words
	// and DROM starts at address 0x3000 instead of
	// 0x1000.
	uint16_t dram[0x3000];
	uint16_t drom[0x800];
	
	#if STATS
	uint64_t instrs_executed;
	uint64_t dma_transfers[4];
	#endif
	
	FileLogger dma_logger;

private:
	void *dma_ptr(uint16_t addr, uint16_t length, bool code);
	
	void skip();
	
	uint16_t fetch();
	
	uint16_t read(uint16_t addr);
	uint16_t read_code(uint16_t addr);
	void write(uint16_t addr, uint16_t value);
	
	void writereg(int reg, uint16_t value);
	
	bool checkcond(int cond);
	void updateflags(uint64_t value);
	
	uint64_t add_with_flags(uint64_t a, uint64_t b);
	
	void trigger_exception(int type);
	void update_timer();
	
	void doext(uint16_t instr);
	
	void add(uint16_t instr);
	void addax(uint16_t instr);
	void addis(uint16_t instr);
	void addr(uint16_t instr);
	void andcf(uint16_t instr);
	void andf(uint16_t instr);
	void andi(uint16_t instr);
	void andr(uint16_t instr);
	void bloop(uint16_t instr);
	void bloopi(uint16_t instr);
	void call(uint16_t instr);
	void callr(uint16_t instr);
	void clr(uint16_t instr);
	void cmp(uint16_t instr);
	void iar(uint16_t instr);
	void ilrr(uint16_t instr);
	void ilrri(uint16_t instr);
	void inc(uint16_t instr);
	void incm(uint16_t instr);
	void jcc(uint16_t instr);
	void jmpr(uint16_t instr);
	void loop(uint16_t instr);
	void lr(uint16_t instr);
	void lri(uint16_t instr);
	void lris(uint16_t instr);
	void lrr(uint16_t instr);
	void lrri(uint16_t instr);
	void lrrn(uint16_t instr);
	void lrs(uint16_t instr);
	void lsl(uint16_t instr);
	void lsr(uint16_t instr);
	void mrr(uint16_t instr);
	void nop(uint16_t instr);
	void orc(uint16_t instr);
	void ori(uint16_t instr);
	void orr(uint16_t instr);
	void ret(uint16_t instr);
	void rti(uint16_t instr);
	void sbclr(uint16_t instr);
	void sbset(uint16_t instr);
	void setam(uint16_t instr);
	void setsu(uint16_t instr);
	void si(uint16_t instr);
	void sr(uint16_t instr);
	void srr(uint16_t instr);
	void srri(uint16_t instr);
	void srrn(uint16_t instr);
	void srs(uint16_t instr);
	void sub(uint16_t instr);
	void subr(uint16_t instr);
	void sxm(uint16_t instr);
	void tst(uint16_t instr);
	void xorc(uint16_t instr);
	void xorr(uint16_t instr);
};
