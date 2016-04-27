#include "queue.h"
#include "htab.h"

/* Some prime numbers <= powers of 2 */
const u_int exp2primes[33] = {
  0x1, /* place holder */
  0x2, 0x3, 0x7, 0xd,
  0x1f, 0x3d, 0x7f, 0xfb,
  0x1fd, 0x3fd, 0x7f7, 0xffd,
  0x1fff, 0x3ffd, 0x7fed, 0xfff1,
  0x1ffff, 0x3fffb, 0x7ffff, 0xffffd,
  0x1ffff7, 0x3ffffd, 0x7ffff1, 0xfffffd,
  0x1ffffd9, 0x3fffffb, 0x7ffffd9, 0xfffffc7,
  0x1ffffffd, 0x3fffffdd, 0x7fffffff, 0xfffffffb,
};

/* Highest bit set in a byte */
static const char bytemsb[0x100] = {
  0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

/* Find last set (most significant bit) */
inline int
fls (u_int32_t v)
{
  if (v & 0xffff0000)
    if (v & 0xff000000)
      return 24 + bytemsb[v>>24];
    else
      return 16 + bytemsb[v>>16];
  if (v & 0x0000ff00)
    return 8 + bytemsb[v>>8];
  else
    return bytemsb[v];
}

/* Ceiling of log base 2 */
int
log2c (u_int32_t v)
{
  return v ? fls (v - 1) : -1;
}
