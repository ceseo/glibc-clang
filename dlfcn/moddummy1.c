/* Provide a dummy DSO for tst-rec-dlopen, tst-dladdr1-handle to use.  */
#include <stdio.h>
#include <stdlib.h>

int
dummy1 (void)
{
  printf ("Called dummy1()\n");
  return 1;
}
