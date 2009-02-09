//[module]	HEAPLIB
//[processor]	IOP
//[type]	ELF-IRX
//[name]	Heap_lib
//[version]	0x101
//[memory map]	-
//[handlers]	-
//[entry point]	heaplib_start, heaplib_stub
//[made by]	[RO]man (roman_ps2dev@hotmail.com) 26 January 2003

#include "sysmem.h"
#include "kloadcore.h"
#include "kheaplib.h"
#include "iopdebug.h"

static int debug=1;

#define _dprintf(fmt, args...) \
	if (debug > 0) __printf("\033[0;32m" "heaplib: " "\033[0m" fmt, ## args)

int _start(int argc, char* argv[]);

///////////////////////////////////////////////////////////////////////
void ll_reset(struct ll *l){
	l->prev=l->next=l;
}

///////////////////////////////////////////////////////////////////////
int ll_one(struct ll *l){
	return (l == l->next);
//	return (l ^ l->next) < 1;	//l==l->next
}

///////////////////////////////////////////////////////////////////////
void ll_remove(struct ll *l){
	l->next->prev=l->prev;
	l->prev->next=l->next;
}

///////////////////////////////////////////////////////////////////////
/*int ll_one_two(struct ll *l){
	return (l->prev ^ l->next) < 1;	//l->prev==l->next
}*/

///////////////////////////////////////////////////////////////////////
void ll_add(struct ll *l, struct ll *n){
	n->next=l;
	n->prev=l->prev;
	l->prev=n;
	n->prev->next=n;
}

void HeapPrepare(void *mem, int size) {
	struct Chunk *_chunk = (struct Chunk*)mem;

	if (mem == NULL || size < 41) return;

	_chunk->_mem=(long)mem-1;
	_chunk->freesize=size;
	_chunk->usedsize=0;
	_chunk->mem_16 = (u32)mem+16;
	_chunk->unk5=((size-16)>>3)-1;
	_chunk->unk4=(long)_chunk+8+((size-16)&~7);
	((struct ll*)_chunk->unk4)->next=(void*)_chunk->mem_16;
	((struct ll*)_chunk->unk4)->prev=0;
}

int HeapChunkSize(void *chunk) {
//	int size = (chunk[1] - 16) >> 3;
//	return ((size - chunk[2]) - 1) << 3;
}

///////////////////////////////////////////////////////////////////////
void* CreateHeap(int chunkSize, int memoryType){
	struct Heap *_heap;

	_dprintf("%s\n", __FUNCTION__);
	chunkSize = (chunkSize+3) & ~3;
	_heap = AllocSysMemory(memoryType & 2 ? ALLOC_LAST : ALLOC_FIRST, chunkSize, NULL);
	if (_heap == NULL) return NULL;

	_heap->plus_one=(long)_heap + 1;
	if (memoryType & 1)	_heap->size2free=chunkSize;
	else                _heap->size2free=0;
	_heap->size2free |= ((memoryType>>1) & 1);
	ll_reset(&_heap->l);

	HeapPrepare(&_heap->mem, chunkSize-4*sizeof(int));
	return (void*)_heap;
}

///////////////////////////////////////////////////////////////////////
int DestroyHeap(void *heap){
	register struct ll *tmp, *p;
	struct Heap *_heap = (struct Heap*)heap;

	if (_heap->plus_one != (long)_heap+1) return (long)_heap+1;
	_heap->plus_one=0;

	for (p=&_heap->l; p != &_heap->l; p=tmp){
		tmp=p->next;
		((int*)p)[2]=0;	//p->mem
		FreeSysMemory(p);
	}
	_heap->mem=0;
	return FreeSysMemory(_heap);
}

///////////////////////////////////////////////////////////////////////
void* HeapMalloc(void *heap, int size) {
#if 0 
	struct Heap *_heap = (struct Heap*)heap;
	void *p;

	if (_heap->plus_one != (long)_heap+1) return NULL;

	for (;;) {
		p = HeapChunk13((u32)_heap->l.next+8, size);
		if (p != NULL) return p;

		if (_heap->l.next->next == _heap->l.next) break;
	}
#endif
}

int HeapFree(void *heap, void *mem) {
	struct Heap *_heap = (struct Heap*)heap;
	struct Heap *h;

	if (heap == NULL || 
		_heap->plus_one != (long)_heap+1) return -4;

	for (h = (struct Heap*)_heap->l.next; h != &_heap->l; h = (struct Heap*)h->l.next) {
		if (HeapChunk14(h->l, mem) != NULL) continue;

		break;
	}

	if ((struct ll*)h == &_heap->l) return 0;
	if (HeapChunk12() == 0) return 0;
	ll_one(h);
	h->l.next = 0;
	FreeSysMemory(h);

	return 0;
}

int HeapSize(void *heap) {
	struct Heap *_heap = (struct Heap*)heap;

	if (heap == NULL || 
		_heap->plus_one != (long)_heap+1) return -4;

	for (;; _heap = _heap->plus_one) {
		if (_heap->l.next == &_heap->l)
			break;
	}

	return HeapChunkSize(_heap->l.next+8);
}

void HeapChunk12() {
}

void HeapChunk13() {
}

void HeapChunk14() {
}

///////////////////////////////////////////////////////////////////////
void _retonly(){}

struct export heaplib_stub={
	0x41C00000,
	0,
	VER(1, 1),	// 1.1 => 0x101
	0,
	"heaplib",
	(func)_start,	// entrypoint
	(func)_retonly,
	(func)_retonly,
	(func)_retonly,
	(func)CreateHeap,
	(func)DestroyHeap,
	(func)HeapMalloc,
	(func)HeapFree,
	(func)HeapSize,
	(func)_retonly,
	(func)_retonly,
	(func)HeapPrepare,
	(func)HeapChunk12,
	(func)HeapChunk13,
	(func)HeapChunk14,
	(func)HeapChunkSize,
	(func)_retonly,
	(func)_retonly,
	0
};


//////////////////////////////entrypoint///////////////////////////////
int _start(int argc, char* argv[]){
	return RegisterLibraryEntries(&heaplib_stub);
}

