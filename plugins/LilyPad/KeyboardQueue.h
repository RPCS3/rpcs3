
// This entire thing isn't really needed,
// but takes little enough effort to be safe...

void QueueKeyEvent(int key, int event);
int GetQueuedKeyEvent(keyEvent *event);

// Cleans up as well as clears queue.
void ClearKeyQueue();
