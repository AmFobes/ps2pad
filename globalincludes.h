#ifndef ___GLOBALINCLUDES_H___
#define ___GLOBALINCLUDES_H___
/*
 Some shared includes and definitions
*/

#define SERVER_PORT_TCP 18196 // 18194 and 18195 are used by ps2link, so why not
#define SERVER_PORT_UDP 18197 // Sharing the same port for udp and tcp is probably harmless, but I'll use two different ports

// Set this too low and your ps2 might DoS your network via udp flood :)
// Set this too high and you're gonna get input lag
#define SERVER_UDP_SEND_DELAY_NS 40000000L

// 512 seems to not crash, setting this too low can have quite undefined behaviour
#define THREAD_STACK_SIZE 512

// If the controller is _not_ a dualshock device ps2pcpad will refuse to use it
#define STOP_ON_NON_DUALSHOCK

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <stdio.h>
#include <string.h>
#include <debug.h>
#include <stdarg.h>
#include <time.h>
#include <libpad.h> 
#include <ps2ips.h>

#define UNULL 0u // Passing NULL as an unsigned argument creates a warning
#endif
