#include <stdio.h>

#include "sno_utility.h"
#include "sno_state.h"
#include "sno_parser.h"
#include "sno_string.h"



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
		sno_free(state, filebuffer);
		fclose(file);
		return NULL;
	}
	sno_assert(readsize <= (size_t)size);
	filebuffer = (char*)sno_realloc(state, filebuffer, readsize + 1);
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



EMSCRIPTEN_EXPORT int main(int argc, char** argv) {
	printf("Hello Sno\n");

	if (argc == 2) {
		printf("With an arg \"%s\"\n", argv[1]);
	}

	sno_State* state = sno_create_state();

	size_t source_code_length = 0;
	const char* const source_code_string = load_string_from_file(
		state,
		"test.sno",
		&source_code_length
	);

	const sno_String* name = sno_create_string_from_literal(state, "test.sno");
	const sno_String* source_code = sno_create_string(state, source_code_string, source_code_length);

	//printf("%.*s\n", (unsigned int)source_code->length, sno_string_chars(source_code));
	
	sno_print_source_code(state, name, source_code);
	
	sno_parse_source_code(state, name, source_code);

	sno_free_state(state);

	return 0;
}
