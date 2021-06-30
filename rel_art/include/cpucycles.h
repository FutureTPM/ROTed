/*
cpucycles amd64cpuinfo.h version 20100803
D. J. Bernstein
Public domain.
*/

#ifndef CPUCYCLES_amd64cpuinfo_h
#define CPUCYCLES_amd64cpuinfo_h

#ifdef __cplusplus
extern "C" {
#endif

extern long long cpucycles_amd64cpuinfo(void);
extern long long cpucycles_amd64cpuinfo_persecond(void);

long long cpucycles_amd64cpuinfo(void)
{
#if defined __x86_64__ || defined __i386__
  unsigned long long result;
  asm volatile(".byte 15;.byte 49;shlq $32,%%rdx;orq %%rdx,%%rax"
    : "=a" (result) ::  "%rdx");
  return result;
#elif defined __arm__
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
#elif defined __aarch64__
  return 0;
#else
#error Undetected arch
#endif
}

#ifdef __cplusplus
}
#endif

#ifndef cpucycles_implementation
#define cpucycles_implementation "amd64cpuinfo"
#define cpucycles cpucycles_amd64cpuinfo
#define cpucycles_persecond cpucycles_amd64cpuinfo_persecond
#endif

#endif
