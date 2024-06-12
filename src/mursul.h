#pragma once

typedef union {
  char word_val[3];
  unsigned int command_val : 12;
} input_union;
