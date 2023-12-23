#include <inttypes.h>
#include <stdio.h>

static uint8_t byte_counts[256] = { 0 };

static void
process(FILE *fp)
{
  int c = -1;

  while ((c = getc(fp)) != -1)
    byte_counts[c]++;
}

static void
print_stats(void)
{
  printf("Hex Statistics:\n");
  for (int i = 0; i < 256; i++)
    printf("\t[%d] : %"PRIu8"\n", i, byte_counts[i]);
}

int
main(int argc, char *argv[])
{
  process(stdin);
  print_stats();

  return 0;
}
