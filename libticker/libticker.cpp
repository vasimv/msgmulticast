#include <stdlib.h>
#include "libticker.h"

Aticker::Aticker(struct step steps[], int nstep) {
	set(steps, nstep);
}

Aticker::Aticker() {
	end = 1;
}

void Aticker::set(struct step steps[], int nstep) {
	mysteps = steps;
	curstep = nstep;
	lastmillis = millis();
	waitfor = 0;
	end = 0;
}

int Aticker::stepcall() {
	long curmillis = millis();

	if ((curmillis - lastmillis) >= waitfor) {
		callnext();
		while (mysteps[curstep].tonext == 0)
			callnext();
	} else
		waitfor -= (curmillis - lastmillis);
	lastmillis = curmillis;
	return curstep;
}

void Aticker::callnext() {
	if (end)
		return;
	(*mysteps[curstep].func)(mysteps[curstep].arg, mysteps[curstep].argpt);
	waitfor = mysteps[curstep].tonext;
	if (waitfor < 0) {
		if (waitfor == ATICKER_STOP)
			end = 1;
		else {
			curstep = abs(waitfor) - 1;
			(*mysteps[curstep].func)(mysteps[curstep].arg, mysteps[curstep].argpt);
			waitfor = mysteps[curstep].tonext;
			curstep++;
		}
		return;
	}
	curstep++;
}

void Aticker::loopback(int arg, void *argpt) {
	curstep = arg;
	waitfor = mysteps[curstep].tonext;
}