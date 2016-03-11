#include <assert.h>
#include <stdio.h>   // printf
#include <stdlib.h>  // NULL
#include <string.h>  // strtok


int parse(char* command, /*out*/ int* argCount, /*out*/ char** argValues) {
	printf("Parsing '%s'\n", command);

	printf("Argv starts at %p\n", argValues);

	printf("Program name:(%p) -> '%s'\n", (void*)token, token);


	printf("Args:\n");
	/* iterate over tokens */
		printf("  * (%p) -> '%s'\n", (void*)token, token);

	printf("---\n");

	return 0; /* OK */
}



/*
 * Eventually there will be a fixed limit
 * Fixing it a-priori makes this task easier
 */
#define MAX_ARGS 127


int main(int argc, char** argv) {

	char test[] = "program arg1 arg2 arg3";
	printf("test@%p ->%p '%s'\n",          &test, test, test);

	int c = 0;
	char* v[MAX_ARGS+1] = {0};
	v[MAX_ARGS] = NULL;


	/* Run parser
	   ---------- */
	parse(test, &c, v);

	/* Test parser output
	   ------------------ */

	printf("\n\n");
	printf("%d arguments loaded\n", c);
	printf("Argv starts at %p\n", &v);
	printf("  Argv[0] @%p ->%p '%s'\n",          &v[0], v[0], v[0]);
	printf("  Argv[1] @%p ->%p '%s'\n",          &v[1], v[1], v[1]);
	printf("  Argv[2] @%p ->%p '%s'\n",          &v[2], v[2], v[2]);
	printf("  Argv[3] @%p ->%p '%s'\n",          &v[3], v[3], v[3]);
	printf("  .\n");
	printf("  Argv[4] @%p ->%p (must be nil)\n", &v[4], v[4]);


	/*
	 * Original memory is NOT owned by new the program, so it should not be
	 * used after parsing
	 */
	printf("\n\n");
	printf("Deleting original string (you MUST make a copy of the string!)\n");
	for(int i=0; i<22; i++)
		test[i] = 'a';
	printf("Original string deleted (%s)\n", test);


	printf("\n\n");
	printf("Testing argCount (%d)\n", c);
	assert(c == 4);

	printf("\n\n");
	printf("testing argv[0] = programName...\n");
	printf("  programName: '%s'\n", v[0]);
	assert(strcmp(v[0], "program")==0);

	printf("\n\n");
	printf("testing argv[1..argc]...\n");
	assert(strcmp(v[1], "arg1")==0);
	assert(strcmp(v[2], "arg2")==0);
	assert(strcmp(v[3], "arg3")==0);

	printf("\n\n");
	printf("testing argv[argc]=NULL...\n");
	assert(v[4] == NULL);

	printf("\n\n");
	printf("All seems to be working" /*random inter-string space*/ " (=\n");
	printf("Now try to put this on pintos and push the values on to the\n"  /* That allows to cut lines */
	       " stack frame correctly (read the docs (rtfm))\n");

	return 0;
}
