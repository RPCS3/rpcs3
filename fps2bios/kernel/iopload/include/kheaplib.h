#ifndef __HEAPLIB_H__
#define __HEAPLIB_H__

struct ll{ struct ll *next, *prev; };			//linked list


struct Heap {
	long plus_one;
	int size2free;
	struct ll l;
	void *mem;
};

struct Chunk {
	u32 _mem;
	int freesize;
	int usedsize;
	u32 mem_16;
	u32 unk4;
	u32 unk5;
};

void *CreateHeap(int chunkSize, int memoryType );
int  DestroyHeap(void *heap);

void *HeapMalloc(void *heap, int size);
int  HeapFree(void *heap, void * mem);
int  HeapSize(void *heap);

#endif /* __HEAPLIB_H__ */
