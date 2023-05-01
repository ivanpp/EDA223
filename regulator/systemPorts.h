#ifndef SYSTEM_PORT
#define SYSTEM_PORT

#include "sciTinyTimber.h"
#include "sioTinyTimber.h"
#include "canTinyTimber.h"

// make Some ports instance global, to debug easily
//  #include "systemPorts.h"
//  to debug with Serial printout information
extern Serial sci0;
extern SysIO sio0;
extern Can can0;

#endif