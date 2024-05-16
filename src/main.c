#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define MAX_GUESSES 5
#define WORD_LENGTH 6 // 5 + \0
#define SAFE_WORD "exit";

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


// TODO: implement actual word fetching from file/url
char* get_word() {
	return "stall";
}


int is_alpha_right_length(char* input) {
	static size_t desierd_len = WORD_LENGTH - 1;
	char* input_p = input;
	size_t len = 0;
	for (char c = *input_p; c; c = *++input_p,++len) {
		if (!isalpha(c))
			return 2;
	}
	if (input_p - input < desierd_len)
		return 1;
	return 0;
}

int get_input(char* fill) {
	static size_t desierd_len = WORD_LENGTH - 1;

	// not matching
	if (scanf("%5s", fill) != 1)
		return -1;

	if (strcasecmp("exit", fill) == 0)
		return -5;

	// longer
	if (getchar() != '\n')
		return -2;

	char* fillP = fill;
	for (char c = *fillP; c; c = *++fillP) {
		// wrong format
		if (!isalpha(c))
			return -4;
	}

	// too short
	if ((size_t)(fillP - fill) < desierd_len)
		return -3;

	return 0;
}

void compare_words(const char* word, char* input) {
	size_t i = 0;
	for (char c = *input; c; c=*++input, ++i) {
		if (c == word[i]) {
			printf("\x1b[1;32m%c\x1b[0m", c);
			continue;
		} 
		if (strchr(word, c) != NULL) {
			printf("\x1b[0;33m%c\x1b[0m", c);
			continue;
		}
		printf("\x1b[0;37m%c\x1b[0m", c);
	}

	printf("\n");
}

int main(void) {
	printf("mursul baby\n");
	printf("--------\n");
	printf("Whenever I’m about to do something, I think, “Would an idiot do that?”\n");
	printf("And if they would, I do not do that thing. - Dwight Schrute.\n");
	printf("Now let's see how much of an idiot are YOU!\n");
	printf("--------\n");


	game game_state = {
		.state = IN_PROGRESS,
		.number_of_guesses = 0,
	};

	char* word_to_match = get_word();
	// game loop
	while(game_state.state == IN_PROGRESS) {
		if (game_state.number_of_guesses == 5)
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

		// game logic here
		if (strcmp(current_word, word_to_match) == 0)
			game_state.state = WON;
		else 
			compare_words(word_to_match, current_word);
		
		// end of game logic

		game_state.number_of_guesses++;

	}

	if (game_state.state == WON) {
		printf("You may not be an idiot, this time...\n");
		return 0;
	}

	printf("defintely and idiot, good luck not being one next time...\n");

	return 0;
}
