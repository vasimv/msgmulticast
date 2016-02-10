#include "libticker.h"

static struct step *steps;
static int curstep;

static long lastmillis;
static int waitfor;

void stepreset(struct step steps[], int nstep) {
	step = steps;
	curstep = nstep;
}

int stepcall() {
	return curstep;
}
