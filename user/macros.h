#include <stdio.h>

#ifdef DEBUG
    #define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
    #define DEBUG_LOG(...)
#endif
