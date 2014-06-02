#include <time.h>
#include <sys/time.h>
#include "GNU.h"

#ifdef __APPLE__
void * _aligned_malloc(size_t size, size_t alignment) {
  void *buffer;
  posix_memalign(&buffer, alignment, size);
  return buffer;
}

int clock_gettime(int foo, struct timespec *ts) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  ts->tv_sec = tv.tv_sec;
  ts->tv_nsec = tv.tv_usec * 1000;
  return(0);
}
#endif /* !__APPLE__ */

#if defined(__GNUG__)
int64_t InterlockedOr64(volatile int64_t *dest, int64_t val)
{
	int64_t olderval;
	int64_t oldval = *dest;
	do
	{
		olderval = oldval;
		oldval = InterlockedCompareExchange64(dest, olderval | val, olderval);
	} while (olderval != oldval);
	return oldval;
}

uint64_t __umulh(uint64_t a, uint64_t b)
{
	uint64_t result;
	__asm__("mulq %[b]" : "=d" (result) : [a] "a" (a), [b] "rm" (b));
	return result;
}

int64_t  __mulh(int64_t a, int64_t b)
{
	int64_t result;
	__asm__("imulq %[b]" : "=d" (result) : [a] "a" (a), [b] "rm" (b));
	return result;
}
#endif