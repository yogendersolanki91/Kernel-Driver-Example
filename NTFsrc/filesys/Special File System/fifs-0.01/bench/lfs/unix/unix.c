#include <windows.h>
#include "unix.h"
#include <fcntl.h>

// struct timeval {
//     long tv_sec;
//     long tv_usec;
// };

static DWORDLONG time_offset = 0;
static const DWORDLONG time_ufactor = 10;
static const DWORDLONG time_factor = 10*1000*1000;
//static BOOL initialized = FALSE;
_fmode = O_BINARY;

int
unix_init()
{
    SYSTEMTIME st;
    st.wYear = 1970;
    st.wMonth = 1;
    st.wDay = 1;
    st.wHour = 0;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    if (!SystemTimeToFileTime(&st, (LPFILETIME)&time_offset)) return 0;
    return 1;
}

int
gettimeofday(
    struct timeval* tp,
    ...
    )
{
    DWORDLONG t;
    GetSystemTimeAsFileTime((LPFILETIME)&t);
    t -= time_offset;
    tp->tv_sec = t / time_factor;
    t %= time_factor;
    tp->tv_usec = t / time_ufactor;
    return 0;
}
