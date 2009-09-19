#include "Global.h"
// This is undoubtedly completely unnecessary.
#include "KeyboardQueue.h"

// What MS calls a single process Mutex.  Faster, supposedly.
// More importantly, can be abbreviated, amusingly, as cSection.
static CRITICAL_SECTION cSection;
static u8 csInitialized = 0;

#define EVENT_QUEUE_LEN 16
// Actually points one beyond the last queued event.
static u8 lastQueuedEvent = 0;
static u8 nextQueuedEvent = 0;
static keyEvent queuedEvents[EVENT_QUEUE_LEN];

void QueueKeyEvent(int key, int event) {
	if (!csInitialized) {
		csInitialized = 1;
		InitializeCriticalSection(&cSection);
	}
	EnterCriticalSection(&cSection);

	// Don't queue events if escape is on top of queue.  This is just for safety
	// purposes when a game is killing the emulator for whatever reason.
	if (nextQueuedEvent == lastQueuedEvent ||
		queuedEvents[nextQueuedEvent].key != VK_ESCAPE ||
		queuedEvents[nextQueuedEvent].evt != KEYPRESS) {
			// Clear queue on escape down, bringing escape to front.  May do something
			// with shift/ctrl/alt and F-keys, later.
			if (event == KEYPRESS && key == VK_ESCAPE) {
				nextQueuedEvent = lastQueuedEvent;
			}

			queuedEvents[lastQueuedEvent].key = key;
			queuedEvents[lastQueuedEvent].evt = event;

			lastQueuedEvent = (lastQueuedEvent + 1) % EVENT_QUEUE_LEN;
			// If queue wrapped around, remove last element.
			if (nextQueuedEvent == lastQueuedEvent) {
				nextQueuedEvent = (nextQueuedEvent + 1) % EVENT_QUEUE_LEN;
			}
	}
	LeaveCriticalSection(&cSection);
}

int GetQueuedKeyEvent(keyEvent *event) {
	if (lastQueuedEvent == nextQueuedEvent) return 0;

	EnterCriticalSection(&cSection);
	*event = queuedEvents[nextQueuedEvent];
	nextQueuedEvent = (nextQueuedEvent + 1) % EVENT_QUEUE_LEN;
	LeaveCriticalSection(&cSection);
	return 1;
}

void ClearKeyQueue() {
	lastQueuedEvent = nextQueuedEvent;
	if (csInitialized) {
		DeleteCriticalSection(&cSection);
		csInitialized = 0;
	}
}
