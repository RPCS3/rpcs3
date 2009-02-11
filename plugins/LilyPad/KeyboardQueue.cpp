// This is undoubtedly completely unnecessary.
#include "KeyboardQueue.h"

static int numQueuedEvents = 0;
static keyEvent queuedEvents[20];

// What MS calls a single process Mutex.  Faster, supposedly.
// More importantly, can be abbreviated, amusingly, as cSection.
static CRITICAL_SECTION cSection;
static int csInitialized = 0;

void QueueKeyEvent(int key, int event) {
	if (!csInitialized) {
		csInitialized = 1;
		InitializeCriticalSection(&cSection);
	}
	EnterCriticalSection(&cSection);
	if (numQueuedEvents >= 15) {
		// Generally shouldn't happen.
		for (int i=0; i<15; i++) {
			queuedEvents[i] = queuedEvents[i+5];
		}
		numQueuedEvents = 15;
	}
	int index = numQueuedEvents;
	// Move escape to top of queue.  May do something
	// with shift/ctrl/alt and F-keys, later.
	if (event == KEYPRESS && key == VK_ESCAPE) {
		while (index) {
			queuedEvents[index-1] = queuedEvents[index];
			index--;
		}
	}
	queuedEvents[index].key = key;
	queuedEvents[index].event = event;
	numQueuedEvents ++;
	LeaveCriticalSection(&cSection);
}

int GetQueuedKeyEvent(keyEvent *event) {
	int out = 0;
	if (numQueuedEvents) {
		EnterCriticalSection(&cSection);
		// Shouldn't be 0, but just in case...
		if (numQueuedEvents) {
			*event = queuedEvents[0];
			numQueuedEvents--;
			out = 1;
			for (int i=0; i<numQueuedEvents; i++) {
				queuedEvents[i] = queuedEvents[i+1];
			}
		}
		LeaveCriticalSection(&cSection);
	}
	return out;
}

void ClearKeyQueue() {
	if (numQueuedEvents) {
		numQueuedEvents = 0;
	}
	if (csInitialized) {
		DeleteCriticalSection(&cSection);
		csInitialized = 0;
	}
}
