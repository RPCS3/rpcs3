#include "GNU.h"

#ifdef __APPLE__
void * _aligned_malloc(size_t size, size_t alignment) {
  void *buffer;
  posix_memalign(&buffer, alignment, size);
  return buffer;
}
#endif
