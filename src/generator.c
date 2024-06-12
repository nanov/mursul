#include "mursul.h"
#include <stdio.h>

#define QUIT_COMMAND ":q"
#define CHEAT_COMMAND ":c"

int main(void) {
  FILE *f = fopen("src/commands.h", "w");
  fprintf(f, "// Auto generated - do not edit\n");
  input_union quit_command = {.word_val = QUIT_COMMAND};
  fprintf(f, "#define QUIT_COMMAND_VALUE %d\n", quit_command.command_val);
  input_union cheat_command = {.word_val = CHEAT_COMMAND};
  fprintf(f, "#define CHEAT_COMMAND_VALUE %d\n", cheat_command.command_val);
  fclose(f);

  return 0;
}
