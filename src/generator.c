#include "mursul.h"
#include <stdio.h>
#include <stdlib.h>

#define QUIT_COMMAND ":q"
#define CHEAT_COMMAND ":c"

int main(void) {
  FILE *f = fopen("src/assets.h", "w");
	FILE *fs = fopen("assets/dwight_idiot.mp3", "r");
	fseek(fs, 0L, SEEK_END);
	size_t sound_size = ftell(fs);
	fseek(fs, 0L, SEEK_SET);
	char* sound_buffer = malloc(sizeof(*sound_buffer) * sound_size);
	fread(sound_buffer, 1, sound_size, fs);
	fclose(fs);
	fprintf(f, "int dwight_idiot = [");
	for(size_t i = 0; i<sound_size;++i) {
		fprintf(f, "%d", *sound_buffer++);
		if (i!=sound_size-1)
			fprintf(f,",");
	}
	fprintf(f, "];\n");


	
  fprintf(f, "// Auto generated - do not edit\n");
  input_union quit_command = {.word_val = QUIT_COMMAND};
  fprintf(f, "#define QUIT_COMMAND_VALUE %d\n", quit_command.command_val);
  input_union cheat_command = {.word_val = CHEAT_COMMAND};
  fprintf(f, "#define CHEAT_COMMAND_VALUE %d\n", cheat_command.command_val);
  fclose(f);

  return 0;
}
