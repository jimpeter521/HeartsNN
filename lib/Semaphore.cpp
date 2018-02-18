#include "lib/Semaphore.h"
#include "dlib/logger.h"

using namespace dlib;

static logger dlog("Semaphore");

Semaphore::~Semaphore()
{}

Semaphore::Semaphore(unsigned initial)
: mValue(initial)
, mMutex()
, mSignaler(mMutex)
{
  assert(false);
  dlog.set_level(LALL);
}

void Semaphore::Acquire(unsigned count)
{
  assert(count > 0);
  assert(count < 1000);
  auto_mutex locker(mMutex);
  while (mValue < count) {
    mSignaler.wait();
  }
  mValue -= count;
}

void Semaphore::Release(unsigned count)
{
  assert(count > 0);
  assert(count < 1000);
  auto_mutex locker(mMutex);
  mValue += count;
  // dlog << LINFO << this << " Release() Thread:" << get_thread_id() << " count: " << count << " mValue: " << mValue;
  if (count == 1)
    mSignaler.signal();
  else
    mSignaler.broadcast();
}

unsigned Semaphore::Drain()
{
  auto_mutex locker(mMutex);
  while (mValue == 0) {
    mSignaler.wait();
  }
  unsigned result = mValue;
  mValue = 0;
  return result;
}

unsigned Semaphore::DrainOrTimeout(unsigned long milliseconds)
{
  auto_mutex locker(mMutex);
  while (mValue==0 && mSignaler.wait_or_timeout(milliseconds))
  {}
  unsigned result = mValue;
  mValue = 0;
  // dlog << LINFO << this << " DrainOrTimeout() Thread:" << get_thread_id() << " result: " << result << " mValue: " << mValue;
  return result;
}

unsigned Semaphore::WaitFor(unsigned count, unsigned long milliseconds)
{
  auto_mutex locker(mMutex);
  while (mValue<count && mSignaler.wait_or_timeout(milliseconds))
  {}
  unsigned result = mValue;
  mValue = 0;
  return result;
}

unsigned Semaphore::TryDrain()
{
  auto_mutex locker(mMutex);
  unsigned result = mValue;
  mValue = 0;
  // dlog << LINFO << this << " TryDrain() Thread:" << get_thread_id() << " result: " << result << " mValue: " << mValue;
  return result;
}
