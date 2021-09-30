#ifndef _kernelev_h_
#define _kernelev_h_

#include "helper.h"

class KernelEv {
	IVTNo ivtNo;
	PCB *pcb;

public:
	KernelEv(IVTNo ivtNo);

	~KernelEv();

	void wait();

	void signal();
};

#endif
