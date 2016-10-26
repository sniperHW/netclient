/*
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _SYSTIME_H
#define _SYSTIME_H

#ifdef _WIN
#include <time.h>
#include <Windows.h>


struct _tagGlobalTickCount_t  
{  
  //API ULONGLONG WINAPI GetTickCount64(void);  
  typedef ULONGLONG (WINAPI *GETTICKCOUNT64)(void);  
  GETTICKCOUNT64 pGetTickCount64;  
  
  //HIGH-RESOLUTION PERFORMANCE COUNTER  
  BOOL bMMTimeValid;  
  LARGE_INTEGER m_Start, m_Freq;  
  
  _tagGlobalTickCount_t()  
  {  
    pGetTickCount64 = NULL;  
    bMMTimeValid = FALSE;
    memset(&m_Start, 0, sizeof(m_Start));  
    memset(&m_Freq, 0, sizeof(m_Freq));  
  
    if((pGetTickCount64 = (GETTICKCOUNT64)GetProcAddress(  
      GetModuleHandle("Kernel32.dll"), "GetTickCount64"))) //API valid  
    {  
      OutputDebugStringA( "GetTickCount64 API Valid\r\n" );  
    }  
    else if( QueryPerformanceCounter(&m_Start)   
      && QueryPerformanceFrequency(&m_Freq) ) //high-resolution count valid  
    {  
      bMMTimeValid = TRUE;  
      OutputDebugStringA( "high-resolution count valid\r\n" );  
    }  
    else //use default time  
    {  
      OutputDebugStringA( "just GetTickCount() support only\r\n" );  
    }  
  }  
  
  ULONGLONG GetTickCount64(void)  
  {  
    if(pGetTickCount64) //api  
    {  
      return pGetTickCount64();  
    }  
    else if(bMMTimeValid) //high-resolution count  
    {  
      LARGE_INTEGER m_End = {{0}};  
      QueryPerformanceCounter(&m_End);  
      return (ULONGLONG)((m_End.QuadPart - m_Start.QuadPart) / (m_Freq.QuadPart / 1000));  
    }  
    else //normal  
    {  
      return GetTickCount();  
    }  
  }  
};  

extern __thread _tagGlobalTickCount_t *GlobalTickCount;       

static inline unsigned long long GetSystemMs64()
{
    if(!GlobalTickCount) 
        GlobalTickCount = new _tagGlobalTickCount_t;
    return GlobalTickCount->GetTickCount64(); 
}

static inline unsigned long GetSystemMs()
{
    return ::GetTickCount();
}

static inline time_t GetSystemSec()
{
    return ::time(NULL);
}

static inline void sleepms(unsigned long ms)
{
    ::Sleep(ms*1000);
}

#else

#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>

#ifndef CLOCK_REALTIME

#include <mach/mach_time.h>
#define CLOCK_REALTIME 1
#define CLOCK_MONOTONIC 2
#define CLOCK_MONOTONIC_RAW 3
static inline int clock_gettime(int clk_id, struct timespec *t){
    
    if(clk_id == CLOCK_MONOTONIC || clk_id == CLOCK_MONOTONIC_RAW){
        mach_timebase_info_data_t timebase;
        mach_timebase_info(&timebase);
        uint64_t time;
        time = mach_absolute_time();
        double seconds = ((double)time * (double)timebase.numer)/((double)timebase.denom * 1e9);
        double nseconds = (seconds - ((double)(uint64_t)seconds)) * 1e9;
        t->tv_sec = seconds;
        t->tv_nsec = nseconds;
    }
    else if(clk_id == CLOCK_REALTIME) {
        struct timeval tm;
        if(0 != gettimeofday(&tm,NULL)){
            return chk_error_common;
        }
        t->tv_sec = tm.tv_sec;
        t->tv_nsec = tm.tv_usec * 1e3;
    }
    else {
        return chk_error_common;
    }

    return chk_error_ok;
}
#endif

extern pthread_key_t g_systime_key;
extern pthread_once_t g_systime_key_once;

struct _clock
{
    uint64_t last_tsc;
    uint64_t last_time;
};

#define NN_CLOCK_PRECISION 1000000

static inline uint64_t _clock_rdtsc ()
{
#if (defined _MSC_VER && (defined _M_IX86 || defined _M_X64))
    return __rdtsc ();
#elif (defined __GNUC__ && (defined __i386__ || defined __x86_64__))
    uint32_t low;
    uint32_t high;
    __asm__ volatile ("rdtsc" : "=a" (low), "=d" (high));
    return (uint64_t) high << 32 | low;
#elif (defined __SUNPRO_CC && (__SUNPRO_CC >= 0x5100) && (defined __i386 || \
    defined __amd64 || defined __x86_64))
    union {
        uint64_t u64val;
        uint32_t u32val [2];
    } tsc;
    asm("rdtsc" : "=a" (tsc.u32val [0]), "=d" (tsc.u32val [1]));
    return tsc.u64val;
#else
    /*  RDTSC is not available. */
    return 0;
#endif
}

static inline uint64_t _clock_time ()
{
    struct timespec tv;
    clock_gettime (CLOCK_REALTIME, &tv);
    return tv.tv_sec * (uint64_t) 1000 + tv.tv_nsec / 1000000;
}

static inline void _clock_init (struct _clock *c)
{
    c->last_tsc = _clock_rdtsc ();
    c->last_time = _clock_time ();
}

static inline struct _clock* get_thread_clock()
{
    struct _clock* c = (struct _clock*)pthread_getspecific(g_systime_key);
    if(!c){
       c = (struct _clock*)calloc(1,sizeof(*c));
       _clock_init(c);
       pthread_setspecific(g_systime_key,c);
    }
    return c;
}


static void systick_once_routine(){
    pthread_key_create(&g_systime_key,NULL);
}

static inline uint64_t GetSystemMs64()
{
    pthread_once(&g_systime_key_once,systick_once_routine);
    uint64_t tsc = _clock_rdtsc ();
    if (!tsc)
        return _clock_time ();

    struct _clock *c = get_thread_clock();

    /*  If tsc haven't jumped back or run away too far, we can use the cached
        time value. */
    if (tsc - c->last_tsc <= (NN_CLOCK_PRECISION / 2) && tsc >= c->last_tsc)
        return c->last_time;

    /*  It's a long time since we've last measured the time. We'll do a new
        measurement now. */
    c->last_tsc = tsc;
    c->last_time = _clock_time ();
    return c->last_time;
}

static inline uint32_t GetSystemMs()
{
    return (uint32_t)GetSystemMs64();
}

static inline time_t GetSystemSec()
{
    return time(NULL);
}

static inline void sleepms(uint32_t ms)
{
    uint64_t endtick = GetSystemMs64()+ms;
    do{
        uint64_t _ms = endtick - GetSystemMs64();
        usleep(_ms*1000);
    }while(GetSystemMs64() < endtick);
}
#endif

#endif
