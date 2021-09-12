#include "helper.h"

volatile PCB *PCB::running = nullptr;
volatile Vector<PCB *> PCB::threads;
volatile ID PCB::globalID = 0;

PCB::PCB(StackSize stackSize, Time timeSlice, Thread *thread, void (*target)(), State state) :
		state(state),
		PCBlocks(0),
		unblTime(false),
		thread(thread),
		timerPasses(timeSlice),
		semaphorTime(0),
		semaphorLeft(0) {

	stackSize /= sizeof(unsigned);

	lock;

	stack = new unsigned[stackSize];

	unlock;

	stack[stackSize - 1] = 0x200;
	stack[stackSize - 2] = FP_SEG(target);
	stack[stackSize - 3] = FP_OFF(target);

	sp = bp = FP_OFF(stack + stackSize - 12);
	ss = FP_SEG(stack + stackSize - 12);

	lock;

	id = globalID++;

	if (state != IDLE) threads.pushf(this);

	unlock;
}

PCB::PCB() : stack(nullptr), thread(nullptr) {
	sp = ss = bp = 0;
	PCBlocks = 0;

	state = RUNNING;
	timerPasses = defaultTimeSlice;

	unblTime = false;
	semaphorTime = semaphorLeft = 0;

	lock;

	id = globalID++;
	threads.pushf(this);

	unlock;
}

PCB::~PCB() {
	if (state == IDLE) return;

	lock;

	delete[] stack;

	for (--threads; threads.get() != this; threads++);
	threads.remove();

	unlock;
}

void PCB::start() {
	lock;

	if (state == INITIALIZING) {
		state = READY;
		Scheduler::put(this);
	}

	unlock;
}

void PCB::worker() {

	running->thread->run();

	lock;

	while (running->waitingForMe.size != 0) {
		PCB *releasePCB = running->waitingForMe.popb();
		releasePCB->unblock();
	}

	running->state = TERMINATING;
	dispatch();

	unlock;
}

void idling() {
	while (true);
}

PCB *PCB::idler() {
	static PCB idle(256, 1, nullptr, idling, IDLE);
	return &idle;
}

void PCB::waitToComplete() {
	lock;

	if (PCB::running != this && state != TERMINATING && state != INITIALIZING && state != IDLE) {
		PCB::running->state = SUSPENDED;
		waitingForMe.pushb((PCB *) PCB::running);
		dispatch();
	}

	unlock;
}

Thread *PCB::getThreadById(ID id) {
	lock;

	for (--threads; threads.get() != nullptr; threads++) if (threads.get()->id == id) {
		unlock;
		return threads.get()->thread;
	}

	unlock;
	return nullptr;
}

void PCB::unblock() {
	lock;

	state = READY;
	Scheduler::put(this);

	unlock;
}

void PCB::block() {
	lock;
	state = SUSPENDED;
	unlock;
}

