#include <stdio.h>

void sub()
{
  fprintf(stdout, "sub() called.\n");
}

int main()
{
  sub();

  return 0;
}
