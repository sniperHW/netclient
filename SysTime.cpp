#include "SysTime.h"

#ifdef _WIN
__thread _tagGlobalTickCount_t *GlobalTickCount = NULL;
#else
pthread_key_t g_systime_key;
pthread_once_t g_systime_key_once = PTHREAD_ONCE_INIT;
#endif
