
#pragma once

#include "class_interpreter.h"
#include <vector>

class Scheduler {
	public:
	typedef std::function<bool()> AlarmCB;
	
	void add(Interpreter *interpreter, int steps);
	bool resume(uint32_t index);
	bool run();

	int currentIndex();
	void addAlarm(uint32_t interval, AlarmCB callback);
	
	private:
	std::vector<Interpreter *> interpreters;
	std::vector<int> steps;
	std::vector<bool> running;
	uint32_t index;
	
	std::vector<uint32_t> alarmIntervals;
	std::vector<uint32_t> alarmTimers;
	std::vector<AlarmCB> alarmCallbacks;
};
