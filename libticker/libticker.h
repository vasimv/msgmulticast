#ifndef __LIB_MSG_TICKER
#define __LIB_MSG_TICKER

#include <inttypes.h>
#include <Arduino.h>

#define ATICKER_LOOPBACK -1
#define ATICKER_STOP -65535

typedef void (*stepfunction)(int, void *);

struct step {
	// Function and its arguments
	stepfunction func;
	int arg;
	void *argpt;
	// Miliseconds to next
	int tonext;
};

class Aticker {
    public: 
	Aticker();
	// Init to step n with program in steps
	Aticker(struct step steps[], int nstep);
	
	// Sets new program and step
	void set(struct step steps[], int nstep);

	// to call from  loop(), returns non-zero (step number) if next step was made
	int stepcall();

	// Simple callback function for looping, arg is number of step
	void loopback(int arg, void *argpt);

    private:
	struct step *mysteps;
	int curstep;

	long lastmillis;
	int waitfor;
	uint8_t end;

	// Calls next step's function
	void callnext();
};
#endif
