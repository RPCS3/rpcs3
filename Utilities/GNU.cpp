#include "GNU.h"

#ifdef __APPLE__
#include <time.h>
#include <sys/time.h>

int clock_gettime(int foo, struct timespec *ts) {
	struct timeval tv;

	gettimeofday(&tv, NULL);
	ts->tv_sec = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000;
	return(0);
}
#endif /* __APPLE__ */
#if defined(__GNUG__)

void * _aligned_malloc(size_t size, size_t alignment) {
	void *buffer;
	return (posix_memalign(&buffer, alignment, size) == 0) ? buffer : 0;
}
#endif

