#include "lib/debug.h"
#include <stdio.h>
#include <execinfo.h>

void printStack(const char* exp, const char* file, int line)
{
  fprintf(stderr, "Assertion failed:%s at %s:%d\n", exp, file, line);
  const int kMaxFrames = 128;
  void* callstack[kMaxFrames];
  const int frames = backtrace(callstack, kMaxFrames);
  const int kStdErr = 2;
  backtrace_symbols_fd(callstack, frames, kStdErr);
}
