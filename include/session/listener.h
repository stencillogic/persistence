#ifndef _LISTENER_H
#define _LISTENER_H

// This is main listener, it awaits for clients and spawns sessions

#include "defs/defs.h"

// creates listener, exits on error
// returns 0 on success, non 0 on error
sint8 listener_create();

#endif
