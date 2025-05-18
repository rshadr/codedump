#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

static bool
is_prime (int64_t n)
{
  if (n < 2)
    return true;

  int64_t isqrt = (int64_t)(sqrt(n));

  for (int64_t i = 2; i < isqrt; ++i)
    if (n % i)
      return false;

  return true;
}

int
main (int argc, char *argv[])
{
  assert( argc == 2 );

  int n = atoi(argv[1]);

  for (int64_t i = 0; i < (int64_t)(n); ++i)
  {
    if (is_prime(i))
      printf("%"PRIi64" ");
  }

  printf("\n");

  return 0;
}

