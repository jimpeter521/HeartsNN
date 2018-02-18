// PredictQueue.h
#pragma once

#include "dlib/threads.h"

class Semaphore
{
public:
  ~Semaphore();

  Semaphore(unsigned initial = 0);

  void Acquire(unsigned count=1);
    // Subtracts count from mValue, but waits if mValue is less than count for other threads to call Release.

  void Release(unsigned count=1);
    // Adds count to mValue, which will release up to count threads waiting on this Semaphore

  unsigned Drain();
    // Acquires all available and returns the count acquired

  unsigned DrainOrTimeout(unsigned long timeout_ms);
    // Waits up to the given timeout_ms and returns all available, which may be zero.

  unsigned WaitFor(unsigned count, unsigned long milliseconds);
    // Waits for count to be available, but only up to the given timeout_ms.
    // The actual number acquired is returned, which may be zero.

  unsigned TryDrain();
    // Atomically acquire all available values, returning the count acquired, which may be zero.
    // Will not block waiting for any to be released.

private:
  volatile unsigned mValue;
	dlib::mutex mMutex;
	dlib::signaler mSignaler;
};
