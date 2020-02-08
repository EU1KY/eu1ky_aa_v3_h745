#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef DEBUG
#include <stdio.h>
#define DBGPRINT(...) printf(__VA_ARGS__)
#else
#define DBGPRINT(...) do { } while(0)
#endif // DEBUG

#endif // _DEBUG_H
