#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

int main(void)
{
	char *s = "\nnu\0num\n";

	printf("%ld\n", strchrnul(s, '\n') - s);

	return 0;
}
