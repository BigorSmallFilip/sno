#include <stdio.h>

#include "sno.h"

int main(int argc, char** argv) {
	sno_assert(argc <= 2);
	if (argc == 1) {
		printf("The Sno Scripting Language\n");
	} else if (argc == 2) {
		printf("Arg = \"%s\"\n", argv[1]);
	}

	return 0;
}
