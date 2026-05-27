#ifndef jvm_h
#define jvm_h

/* jvm.h — Internal VM interfaces shared by jdo.c and jcode.c. */

#include "jass.h"
#include "jparser.h"

VMPROGRAM VM_Compile(LPCTOKEN token);

#endif /* jvm_h */