// Copyright 2000-2021 Mark H. P. Lord

// Include this from a .cpp to enable spew-logging for that particular .cpp. If not included, the macros are
// defined by Common.h as no-ops (i.e., spew-logging is disabled).
// If you see a file with //#include "Spew.h" at the top, chances are uncommenting it will add a ton of logging.

#undef PRIME_SPEW
#ifndef PRIME_NO_VA_ARGS
#define PRIME_SPEW(...) PRIME_TRACE(__VA_ARGS__)
#else
#define PRIME_SPEW PRIME_TRACE
#endif
#undef PRIME_IF_SPEW
#define PRIME_IF_SPEW(x) x
#define PRIME_SPEW_ENABLED
