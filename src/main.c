#include <curl/curl.h>
#include <sys/ioctl.h>
#include <raylib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define MAX_GUESSES 5
#define WORD_LENGTH 6 // 5 + \0
#define QUIT_COMMAND 'q'
#define CHEAT_COMMAND 'c'
#define ASCII_RANGE (('z' - 'a') + 1)
#define ASCII_POS(c) (c - 'a')

#define ASCII_CLEAR_LINE "\033[2K"
#define ASCII_PREVIOUS_LINE "\x1B[1A"
#define ASCII_ITALIC "\e[3m"
#define ASCII_BOLD "\e[1m"
#define ASCII_RESET "\e[0m"
#define ASCII_GREEN "\x1b[0;32m"
#define ASCII_YELLOW "\x1b[0;33m"
#define INPUT_INPUT_MODE 0
#define INPUT_COMMAND_MODE 2

typedef enum { WON, LOST, IN_PROGRESS } game_phase;

typedef struct {
  char *word;
  game_phase state;
  size_t number_of_guesses;
  char guesses[MAX_GUESSES][WORD_LENGTH];
  struct termios original_terminos;
  struct termios raw_terminos;
} game;

size_t data_from_nyt(char* buffer, size_t item_size, size_t number_of_items,
                     void *word_opaque) {
  size_t total_size = item_size * number_of_items;
  char *word = (char *)word_opaque;
  buffer = strstr(buffer, "solution\":\"");
  if (buffer != NULL) {
    buffer += strlen("solution\":\"");
    strncpy(word, buffer, 5);
    word[WORD_LENGTH - 1] = 0;
  }
  return total_size;
}

int download_word_from_nyt(char *word) {
  time_t now;
  char today_iso[sizeof("1982-09-25")];
  //  https://www.nytimes.com/svc/wordle/v2/2024-05-21.json
  const size_t url_size =
      sizeof("https://www.nytimes.com/svc/wordle/v2/1982-09-25.json");
  char url[url_size];
  CURL *curl;

  time(&now);
  strftime(today_iso, sizeof(today_iso), "%F", gmtime(&now));
  snprintf(url, url_size, "https://www.nytimes.com/svc/wordle/v2/%s.json",
           today_iso);

  // TODO: error handling
  // TODO: logging
  curl = curl_easy_init();
  if (!curl)
    return 1;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_from_nyt);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, word);
  CURLcode result = curl_easy_perform(curl);
  curl_easy_cleanup(curl);

  if (result != CURLE_OK)
    return 1;

  return 0;
}

char *get_word_from_file(char *file_path, char *word) {
  FILE *file;
  time_t t;

  file = fopen(file_path, "r"); // defer fclose
  fseek(file, 0L, SEEK_END);
  srand((unsigned)time(&t));
  size_t num_of_lines = ftell(file) / 6;
  size_t line_number = rand() % num_of_lines;
  fseek(file, line_number * 6, SEEK_SET);

  fgets(word, 6, file);

  word[5] = 0; // null terminator or segfault :/
  fclose(file);

  return word;
}

void init_terminal_state(game *state) {
  tcgetattr(0, &state->original_terminos);
  state->raw_terminos =
      state->original_terminos; /* make new settings same as old settings */
  state->raw_terminos.c_lflag &= ~ICANON; /* disable buffered i/o */
  state->raw_terminos.c_lflag &= ~ECHO;   /* set echo mode */
}

int get_input(game *state, char *fill, char history[5][6],
              size_t history_length) {

  tcsetattr(0, TCSANOW, &state->raw_terminos);
  char f;
  bool dirty = 0; // 1 = normal, 2 = command
  int mode = 0;   // 0 = normal, 2 = command
  size_t pos = 0;
  size_t len = 0;
  size_t history_pos = history_length;
  *fill = '\0';
  while (read(STDIN_FILENO, &f, 1) && f != '\n') {
    switch (f) {
    case 127: { // backspace
      if (pos == 0)
        continue;
      char *pos_ptr = fill + pos;
      memmove(pos_ptr - 1, pos_ptr, 6 - pos);
      pos--;
      len--;
      if (len == 0) {
        mode = 0; // reset mode
        dirty = 0;
      }
    }; break;
    case 8:
      goto move_left; // ctrl+h
    case 12:
      goto move_right; // ctrl+l
    case 27: {             // arrow keys
      read(STDIN_FILENO, &f, 1);
      read(STDIN_FILENO, &f, 1);
      switch (f) {
      case 68:
			move_left:
        if (pos > 0)
          pos--;
        break;
      case 67:
			move_right:
        if (pos < len)
          pos++;
        break;
      case 65:
        if (history_pos == 0)
          continue;
        history_pos--;
        goto history_move;
      case 66:
        if (history_pos >= history_length)
          continue;
        history_pos++;
        goto history_move;
      history_move:
        if (dirty == 1)
          continue;

        if (history_pos == history_length) {
          fill[0] = '\0';
          pos = 0;
          len = 0;
          mode = 0;
        } else {
          strncpy(fill, history[history_pos], 6);
          pos = len = strlen(fill);
          mode = fill[0] == ':' ? INPUT_COMMAND_MODE : 0;
        }
      }
      break;
    case ':':
      if (len != 0)
        continue;
      mode = INPUT_COMMAND_MODE;
    case 'a' ... 'z':
    case 'A' ... 'Z':
      if (len + mode >= 5)
        continue;
      char *pos_ptr = fill + pos;
      memmove(pos_ptr + 1, pos_ptr, 6 - (pos + 1));
      *pos_ptr = f;
      pos++;
      len++;
      dirty = 1;
      break;
    };
    }
    printf(ASCII_CLEAR_LINE "\r%s", fill);
    printf("\033[%zuG", pos + 1);
    fflush(stdout);
  }
  tcsetattr(0, TCSANOW, &state->original_terminos);
  printf("\n");

  if (mode == 0)
    return len == 5 ? 0 : -3;

  if (fill[1] == QUIT_COMMAND)
    return -5;

  if (fill[1] == CHEAT_COMMAND)
    return -6;

  return -7;
}

int compare_words_alt(char *word, char *input) {
  int number_of_right_letters = 0;
  // 'z'(122) - 'a'(97) + 1 == 26
  int secret_count[ASCII_RANGE] = {0};

  for (char *c = word, *in = input; *c; ++c, ++in) {
    if (*in != *c) {
      secret_count[ASCII_POS(*c)]++;
    }
  }

  printf(ASCII_PREVIOUS_LINE);
  for (char *c = word, *in = input; *c; ++c, ++in) {
    if (*in == *c) {
      number_of_right_letters++;
      printf(ASCII_GREEN "%c", *in);
      continue;
    }
    if (secret_count[*in - 'a'] > 0) {
      printf(ASCII_YELLOW "%c", *in);
      secret_count[ASCII_POS(*in)]--;
      continue;
    }
    printf(ASCII_RESET "%c", *in);
  }
  printf(ASCII_RESET "\n");
  return number_of_right_letters;
}

void print_usage() {
  printf("USAGE: mursul [option]\n");
  printf("    --help, -h 						"
         "		 - this\n");
  printf("    --nyt, -n 						"
         "		 - download word from New York Time API\n");
  printf("    --file, -f 						"
         "		 - choose random word from words.txt file in assets\n");
  printf("    --assets=[path], -a=[path] - specify assets path\n");
  printf("\n");
}

int parse_args(int argc, char **argv, char **assets_path) {
  int result = 0;
  for (size_t i = 1; i < (size_t)argc; ++i) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_usage();
      return -1; // exit - help
    } else if (strcmp(argv[i], "--file") == 0 || strcmp(argv[i], "-f") == 0) {
      result = 1;
    } else if (strncmp("--assets=", argv[i], strlen("--assets=")) == 0 ||
               strncmp("-a=", argv[i], strlen("-a=")) == 0) {
      *assets_path = (strchr(argv[i], '=') + 1);
    }
  }
  return result;
}

static game game_state = {
      .state = IN_PROGRESS,
      .number_of_guesses = 0,
  };

typedef struct {
	unsigned short width;
	size_t prompt_pos;
	char prompt[1024];
} terminal_info;

static terminal_info term_info = { .prompt_pos = 0 };
#define SPIT_PRINT(C) sprintf(&term_info.prompt[term_info.prompt_pos], C); \
											 term_info.prompt_pos += strlen(C)
#define SPIT_PRINTF(C, ...) sprintf(&term_info.prompt[term_info.prompt_pos], C, __VA_ARGS__); \
											 term_info.prompt_pos += strlen(C)
#define SPIT_OUTPUT() printf("%s", term_info.prompt); \
											printf("\33[%dG (%zu/%d)\n", term_info.width-6, game_state.number_of_guesses, MAX_GUESSES); \
											fflush(stdout)


void fill_terminal_info(int sig) {
	(void) sig;

	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	unsigned short p_w = term_info.width;	
	term_info.width = w.ws_col;
	if (p_w > term_info.width) {
		printf(ASCII_CLEAR_LINE ASCII_PREVIOUS_LINE ASCII_CLEAR_LINE ASCII_PREVIOUS_LINE ASCII_CLEAR_LINE);
	} else {
		printf(ASCII_CLEAR_LINE ASCII_PREVIOUS_LINE ASCII_CLEAR_LINE);
	}
	SPIT_OUTPUT();

	// TODO: handle screen shrink
}

static char prompt[1028];



int main(int argc, char **argv) {
	fill_terminal_info(0);
	signal(SIGWINCH, fill_terminal_info);
  init_terminal_state(&game_state);
	// TODO: move into one function
  tcsetattr(0, TCSANOW, &game_state.raw_terminos);

  char word_to_match[WORD_LENGTH];

  char *assets_path = "assets/";
  char asset_full_path[4096 + 12];

  printf("mursul baby\n");
  printf("--------\n");
  int c = parse_args(argc, argv, &assets_path);
  if (c == -1) {
    return 0;
  } else if (c == 1) {
    snprintf(asset_full_path, sizeof(asset_full_path), "%s/%s", assets_path, "words.txt");
    get_word_from_file(asset_full_path, word_to_match);
  } else {
    download_word_from_nyt(word_to_match);
  }

  printf(ASCII_ITALIC "Whenever I’m about to do something, I think, “Would an "
                      "idiot do that?”\n");
  printf("And if they would, I do not do that thing. - Dwight Schrute.\n");
  printf("Now let's see how much of an idiot are YOU!\n" ASCII_RESET);
  printf("--------\n");

  SetTraceLogLevel(LOG_ERROR);
  InitAudioDevice();
  snprintf(asset_full_path, sizeof(asset_full_path), "%s/%s", assets_path,
           "dwight_idiot.mp3");
  Sound sound = LoadSound(asset_full_path);




  // game loop
  while (game_state.state == IN_PROGRESS) {
    if (game_state.number_of_guesses == MAX_GUESSES)
      break;

		SPIT_PRINT("please enter a 5 letter word:");
		// SPIT_PRINTF("\33[%dG (%zu/%d)\n", term_info.width-6, game_state.number_of_guesses, MAX_GUESSES);
		SPIT_OUTPUT();

    char *current_word = game_state.guesses[game_state.number_of_guesses];

    int input_result = get_input(&game_state, current_word, game_state.guesses,
                                 game_state.number_of_guesses);
		term_info.prompt_pos = 0;
    switch (input_result) {
    case -7:
      SPIT_PRINTF("Unknown command, idiot! (%zu)\n", game_state.number_of_guesses);
      continue;
    case -6:
      SPIT_PRINTF("Not only and idiot, a cheater as well, shhhh, this is the "
             "word " ASCII_ITALIC ASCII_BOLD "%s" ASCII_RESET "\n",
             word_to_match);
      continue;
    case -5:
      printf("Quitter, idiot.\n");
      return input_result;
    case -1:
    case -4:
      printf("Wrong format, idiot.\n");
      return input_result;
    case -3:
      printf("too short, five letter, idiot.\n");
      continue;
    case -2:
      printf("too long, five letter, idiot.\n");
      continue;
    }

    int guessed_right = compare_words_alt(word_to_match, current_word);
    if (guessed_right == WORD_LENGTH - 1)
      game_state.state = WON;

    // end of game logic

    game_state.number_of_guesses++;
  }

  if (game_state.state == WON) {
    printf("You may not be an idiot, this time...\n");
    return 0;
  }

  printf("defintely and idiot, good luck not being one next time...\n");

  PlaySound(sound);
  // TODO: find better way to block until sound is played
  while (IsSoundPlaying(sound)) {
  }

  UnloadSound(sound);
  CloseAudioDevice();

  return 0;
}
