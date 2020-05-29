#include <stdio.h>
#include <sys/types.h>

#if defined __x86_64__ || defined __i386__
long long cpucycles_amd64cpuinfo(void)
{
  unsigned long long result;
  asm volatile(".byte 15;.byte 49;shlq $32,%%rdx;orq %%rdx,%%rax"
    : "=a" (result) ::  "%rdx");
  return result;
}
#elif defined __arm__
long long cpucycles_amd64cpuinfo(void)
{
  unsigned cc;
  static int init = 0;
  if(!init) {
      __asm__ __volatile__ ("mcr p15, 0, %0, c9, c12, 2" :: "r"(1<<31)); /* stop the cc */
      __asm__ __volatile__ ("mcr p15, 0, %0, c9, c12, 0" :: "r"(5));     /* initialize */
      __asm__ __volatile__ ("mcr p15, 0, %0, c9, c12, 1" :: "r"(1<<31)); /* start the cc */
      init = 1;
  }
  __asm__ __volatile__ ("mrc p15, 0, %0, c9, c13, 0" : "=r"(cc));
  return cc;
}
#else
#error Undetected arch
#endif
