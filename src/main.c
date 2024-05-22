#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <raylib.h>
#include <time.h>
#include <sys/_types/_ssize_t.h>
#include <curl/curl.h>

#define MAX_GUESSES 5
#define WORD_LENGTH 6 // 5 + \0
#define SAFE_WORD "exit"

typedef enum {
	WON,
	LOST,
	IN_PROGRESS
} game_state;

typedef struct {
	char* word;
	game_state state;
	size_t number_of_guesses;
	char guesses[MAX_GUESSES][WORD_LENGTH];
} game;

size_t data_from_nyt(char* buffer, size_t item_size, size_t number_of_items, void* word_opaque) {
	size_t total_size = item_size * number_of_items;
	char* word = (char*)word_opaque;
	buffer = strstr(buffer,"solution\":\"");
	if (buffer != NULL) {
		// TODO: verify that at -O2 and above strlen is compuile time...
		buffer += strlen("solution\":\"");
		strncpy(word, buffer, 5);
		word[WORD_LENGTH -1] = 0;
	}
	return total_size;
}

int download_word_from_nyt(char* word) {
	time_t now;
	char today_iso[sizeof("1982-09-25")];
	//  https://www.nytimes.com/svc/wordle/v2/2024-05-21.json
	char url[sizeof("https://www.nytimes.com/svc/wordle/v2/1982-09-25.json")];
	CURL* curl;

	time(&now);
	strftime(today_iso, sizeof(today_iso), "%F", gmtime(&now));
	snprintf(url, sizeof(url), "https://www.nytimes.com/svc/wordle/v2/%s.json", today_iso);

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

	return 0;
}


char* get_word_from_file(char* file_path, char* word) {
	FILE* file;
	time_t t;

	file = fopen(file_path, "r"); // defer fclose
	fseek(file, 0L, SEEK_END);
	srand((unsigned) time(&t));
	size_t num_of_lines = ftell(file) / 6;
	size_t line_number = rand() % num_of_lines;
	fseek(file, line_number * 6, SEEK_SET);
	

	fgets(word, 7, file);

	word[5] = 0;
	printf("chose = %s\n", word);
	fclose(file);

	return word;
}

int get_input(char* fill) {
	static size_t desierd_len = WORD_LENGTH - 1;

	// not matching
	if (scanf("%5s", fill) != 1)
		return -1;

	if (strcasecmp(SAFE_WORD, fill) == 0)
		return -5;

	// longer
	if (getchar() != '\n')
		return -2;

	char* c;
	for (c = fill; *c; ++c) {
		// wrong format
		if (!isalpha(*c))
			return -4;
		*c = tolower(*c);
	}

	// too short
	if (((size_t)(c - fill)) < desierd_len)
		return -3;

	return 0;
}


int compare_words_alt(char* word, char* input) {
	int number_of_right_letters = 0;
	int secret_count['z' - 'a'] = {0};

	for (char *c=word, *in=input;*c;++c,++in) {
		if (*in != *c) {
			secret_count[*c - 'a']++;
		}
	}

	for (char* c=word, *in = input;*c;++c,++in) {
		if (*in == *c) {
			number_of_right_letters++;
			printf("\x1b[1;32m%c\x1b[0m", *in);
			continue;
		}
		if (secret_count[*in - 'a'] > 0) {
			printf("\x1b[0;33m%c\x1b[0m", *in);
			secret_count[*in - 'a']--;
			continue;
		}
		printf("\x1b[0;37m%c\x1b[0m", *in);
	}
	printf("\n");
	return number_of_right_letters;
}

void print_usage() {
	printf("USAGE: mursul [option]\n");
	printf("    --help, -h - this.\n");
	printf("    --nyt, -n - download word from New York Time API\n");
	printf("    --file, -f - choose random word from words.txt file in assets\n");
}

int main(int argc, char** argv) {
	char word_to_match[WORD_LENGTH];

	printf("mursul baby\n");
	printf("--------\n");
	if (argc < 2) {
		download_word_from_nyt(word_to_match);
	} else {
		if (strcmp(argv[1],"--help")==0 || strcmp(argv[1],"-h")==0) {
			print_usage();
			return 0;
		} else if (strcmp(argv[1],"--file")==0 || strcmp(argv[1],"-f")==0) {
			get_word_from_file("assets/words.txt");
		}
	}

	printf("Whenever I’m about to do something, I think, “Would an idiot do that?”\n");
	printf("And if they would, I do not do that thing. - Dwight Schrute.\n");
	printf("Now let's see how much of an idiot are YOU!\n");
	printf("--------\n");

	SetTraceLogLevel(LOG_ERROR);
	InitAudioDevice();
	Sound sound = LoadSound("assets/stupid.mp3");


	game game_state = {
		.state = IN_PROGRESS,
		.number_of_guesses = 0,
	};

	// game loop
	while(game_state.state == IN_PROGRESS) {
		if (game_state.number_of_guesses == MAX_GUESSES)
			break;
		printf("please enter a 5 letter word:\n");
		char* current_word = game_state.guesses[game_state.number_of_guesses];

		int input_result = get_input(current_word);
		switch (input_result) {
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
		if (guessed_right == WORD_LENGTH -1)
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
	while(IsSoundPlaying(sound)) {}


	UnloadSound(sound);
	CloseAudioDevice();

	return 0;
}
