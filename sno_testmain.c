#include <stdio.h>
#include <string.h>

#include "sno_utility.h"
#include "sno_state.h"
#include "sno_parser.h"
#include "sno_string.h"
#include "sno_gc.h"



#ifndef sno_EMSCRIPTEN

static char* load_string_from_file(sno_State* state, const char* filename, size_t* out_length) {
	sno_assert_ptr(filename);
	sno_assert_ptr(out_length);
	*out_length = 0;
	FILE* file = fopen(filename, "r");
	if (!file) {
		return NULL;
	}
	fseek(file, 0L, SEEK_END);
	long size = ftell(file);
	sno_assert(size >= 0);
	rewind(file);
	sno_assert(size >= 0);
	char* filebuffer = (char*)sno_malloc(state, (size_t)size);
	if (!filebuffer) {
		fclose(file);
		return NULL;
	}
	size_t readsize = fread(filebuffer, sizeof(char), (size_t)size, file);
	sno_assert(readsize <= ULONG_MAX);
	if (ferror(file) != 0) {
		sno_free(state, filebuffer, (size_t)size);
		fclose(file);
		return NULL;
	}
	sno_assert(readsize <= (size_t)size);
	filebuffer = (char*)sno_realloc(state, filebuffer, size, readsize + 1);
	sno_assert_ptr(filebuffer);
	filebuffer[readsize] = '\0';
	fclose(file);
	*out_length = (size_t)readsize;
	return filebuffer;
}

#else

const char* const test_program =
""						  "\n"
"function a() {"		  "\n"
"    for i = 0, 10 {"	  "\n"
"        print(i)"		  "\n"
"    }"					  "\n"
"    if true == true {"	  "\n"
"        print(\"what\")" "\n"
"    }"					  "\n"
"}"						  "\n"
""						  "\n"
"var a = none"			  "\n";

static const char* load_string_from_file(sno_State* state, const char* filename, size_t* out_length) {
	*out_length = sizeof(test_program) - 1;
	return test_program;
}

#endif



static sno_Bool run_file(
	sno_State* state,
	const char* const file_path
) {
	if (!sno_run_file(state, file_path, strlen(file_path))) {
		sno_print_exception_msg(state);
		return sno_FALSE;
	}
	return sno_TRUE;
}



EMSCRIPTEN_EXPORT int main(int argc, char** argv) {
	if (argc == 2) {
	} else {
		printf("The Sno Programming Language... lol\n");
		return 0;
	}

	//printf("%llu be %llu", 1024, sno_smallest_power_of_2_greater_than_or_equal_to(1024));
	
	sno_State* state = sno_create_state();

#ifdef sno_DEBUG
	run_file(state, "SnoTests\\tables.sno");
#endif

	if (!sno_run_file(state, argv[1], strlen(argv[1]))) {
		sno_print_exception_msg(state);
		return -1;
	} else {

	}

	sno_full_gc(state);

	//printf("%.*s\n", (unsigned int)source_code->length, sno_string_chars(source_code));
	
	//sno_print_source_code(state, name, source_code);
	
	sno_free_state(state);

	return 0;
}
