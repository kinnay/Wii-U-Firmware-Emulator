
#include "class_scheduler.h"
#include "errors.h"

void Scheduler::add(Interpreter *interpreter, int steps) {
	interpreters.push_back(interpreter);
	this->steps.push_back(steps);
	running.push_back(false);
}

bool Scheduler::resume(uint32_t index) {
	if (index < running.size()) {
		running[index] = true;
		return true;
	}
	IndexError("Index out of range");
	return false;
}

bool Scheduler::run() {
	while (true) {
		for (index = 0; index < interpreters.size(); index++) {
			if (running[index]) {
				if (!interpreters[index]->run(steps[index])) return false;
			}
		}
		
		index = 0;
		for (uint32_t i = 0; i < alarmTimers.size(); i++) {
			alarmTimers[i]--;
			if (alarmTimers[i] == 0) {
				alarmCallbacks[i]();
				alarmTimers[i] = alarmIntervals[i];
			}
		}
	}
}

int Scheduler::currentIndex() {
	return index;
}

void Scheduler::addAlarm(uint32_t interval, AlarmCB callback) {
	alarmIntervals.push_back(interval);
	alarmTimers.push_back(interval);
	alarmCallbacks.push_back(callback);
}
