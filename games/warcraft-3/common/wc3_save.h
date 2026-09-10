#ifndef BZ_WC3_SAVE_H
#define BZ_WC3_SAVE_H
#include "common/state.h"

/* Reference-domain identities are game contracts, not engine field kinds. */
enum { F_LSTRING = F_CUSTOM, F_GSTRING, F_EDICT, F_ITEM, F_TRIGGER, F_TIMER, F_EVENT,
    F_FUNCTION, F_FUNCTION_LIST, F_CFUNCTION, F_MMOVE };
#endif
