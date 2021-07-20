// Copyright 2000-2021 Mark H. P. Lord

// Include this from a .cpp to enable even-more-spew-logging for that particular .cpp. If not included, the macros are
// defined by Common.h as no-ops (i.e., even-more-spew-logging is disabled).
// If you see a file with //#include "Spew2.h" at the top, chances are uncommenting it will add a ton of logging.

#include "Spew.h"
#undef PRIME_SPEW2
#ifndef PRIME_NO_VA_ARGS
#define PRIME_SPEW2(...) PRIME_TRACE(__VA_ARGS__)
#else
#define PRIME_SPEW2 PRIME_TRACE
#endif
#undef PRIME_IF_SPEW2
#define PRIME_IF_SPEW2(x) x
#define PRIME_SPEW2_ENABLED
