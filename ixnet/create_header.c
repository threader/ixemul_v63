#define _KERNEL
#include "ixnet.h"
#include <stddef.h>
#include <stdio.h>

#ifdef __PPC__

int MY_IXNETBAES_SIZEOF = (int)(sizeof (struct ixnet_base) - offsetof (struct ixnet_base, ix_seg_list) - 4);

#else
int main(int argc, char **argv)
{
  printf ("/* This header has been generated by the create_header tool.\n   DO NOT EDIT! */\n\n");

  printf ("#define IXNETBASE_SIZEOF (IXNETBASE_C_PRIVATE + %d)\n",
	  (int)(sizeof (struct ixnet_base) - offsetof (struct ixnet_base, ix_seg_list) - 4));
  return 0;
}
#endif

