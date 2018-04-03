// lib/DebugStats.h

#pragma once

#include <algorithm>
#include <stdio.h>

#define DEBUG_STATS 0

class DebugStats
{
public:
  DebugStats(const char* label) : mLabel(label), mSum1(0), mSum2(0), mMin(1.0e6), mMax(-1.0e6), mNum(0) {}

  ~DebugStats() { Print(); }

  void Print() {
    #if DEBUG_STATS
    if (mNum > 0) {
      float avg = mSum1 / mNum;
      float std = sqrt(mSum2/mNum - avg*avg);
      printf("%s Mean: %4.2f, std: %4.2f, min: %4.2f, max: %4.2f\n", mLabel, avg, std, mMin, mMax);
    }
    #endif
  }

  inline void Accum(float x) {
    #if DEBUG_STATS
    mSum1 += x;
    mSum2 += x*x;
    mMin = std::min(mMin, x);
    mMax = std::max(mMax, x);
    mNum += 1;
    #endif
  }

private:
  const char* mLabel;
  float mSum1;
  float mSum2;
  float mMin;
  float mMax;
  int   mNum;
};
