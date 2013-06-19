#include <stdio.h>

void puts_alternative(char *arg)
{
  puts("puts_alternative() has been reached.");

  puts(arg);

  puts("puts_alternative() is returning.");
}
