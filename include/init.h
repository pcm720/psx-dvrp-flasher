#ifndef _INIT_H_
#define _INIT_H_

#include <stdint.h>

// Initializes IOP modules
int initModules();


// Resets the DVRP
int xdvrpReset(uint8_t iplMode);

#endif
