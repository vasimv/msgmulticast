#ifndef __LIB_MSG_TICKER
#define __LIB_MSG_TICKER
typedef void (*stepfunction)(int, void *)

struct step {
	// Function and its arguments
	stepfunction func;
	int arg;
	void *argpt;
	// Miliseconds to next
	uint16_t tonext;
};

// Init to step n with program in steps
void stepreset(struct step steps[], int nstep);

// to call from  loop(), returns non-zero (step number) if next step was made
int stepcall()
#endif
