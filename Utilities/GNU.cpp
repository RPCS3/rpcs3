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
