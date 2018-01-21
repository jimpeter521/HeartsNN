


#include <stdint.h>
#include <x86intrin.h>
#include <stdio.h>

int main()
{
  uint64_t x = 8;
  for (int i=0; i<5; ++i) {
    printf("lzcnt(0x%llx) = %llu\n", x, __lzcnt64(x));
    printf("tzcnt(0x%llx) = %llu\n", x, __tzcnt_u64(x));
    x <<= 1;
  }

  return 0;
}
