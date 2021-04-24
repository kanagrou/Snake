/* ANSI HEADERS */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/* COMPILER HEADERS */
#include <conio.h>

/* PLATFORMS HEADERS */
#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
	#include <windows.h>
	#define SLEEP(ms) Sleep(ms)
#else
	#include <ncurses.h>
	#include <unistd.h>
	#define SLEEP(ms) usleep(ms)
	int kbhit() {
		int c,result;

		nodelay(stdscr, TRUE);
		noecho();

		c = getch();
		if (c == ERR)
			result = FALSE;
		else {
			result = TRUE;
			ungetch(c);
		}

		echo();
		nodelay(stdscr, FALSE);
		return(0);
	}
#endif

/* GAME CONSTANTS */
#define CHARS_BORDER 	254 // ■
#define CHARS_SNAKE  	254 // ■
#define CHARS_FOOD		254 // ■
#define AREA_WIDTH   	10
#define AREA_HEIGHT  	10
#define UI_WIDTH		6
#define UI_HEIGHT		6
#define SNAKE_MAX    	(AREA_WIDTH-2) * (AREA_HEIGHT-2)
#define SNAKE_GROWTH	1
#define TICK 			100

/* KEYS CONSTANTS */
#define KEYS_RIGHT 		0x64 // d
#define KEYS_LEFT  		0x61 // a
#define KEYS_UP    		0x77 // w
#define KEYS_DOWN  		0x73 // s
#define KEYS_QUIT  		0x71 // q

/* DIRECTION CONSTANTS */
#define D_RIGHT			0x01
#define D_LEFT			0x02
#define D_UP			0x03
#define D_DOWN			0x04

/* COLLISIONS CONSTANT */
#define C_ITSELF		0x01
#define C_BORDER 		0x02
#define C_FOOD			0x03

/* STATES CONSTANT */
#define S_ALIVE			0x00
#define S_DEAD			0x01
#define S_WIN			0x02


void debug(int val, int col)
{
	printf("\033[%d;%dH%d\n", AREA_HEIGHT+1, col, val);
}

void drawBorders()
{
	printf("\033[H");							// Goto 0,0 before printing
	// printf("\033[1m");							// Bold

	for (int y=0;y<AREA_HEIGHT;++y) {
		for (int x=0;x<AREA_WIDTH;++x) {
			if (y == 0 || y == AREA_HEIGHT-1)
				printf("%c ", CHARS_BORDER);	// Print the whole row
			else {
				printf("%c\033[%dC%c",
						CHARS_BORDER,
						AREA_WIDTH*2-3,
						CHARS_BORDER);
				break;							// Only print the 2 sides
			}
		}
		printf("\n");							// New line after row
	}
	printf("\033[0m");							// Reset bold
}
void drawUI(int vcount, int *vvalue, char *vname[]) {
	printf("\033[0;%dH", AREA_WIDTH*2+2);		// Goto right side before printing
	
	for (int y=0;y<UI_HEIGHT;++y) {
		for (int x=0;x<UI_WIDTH;++x) {
			if (y == 0 || y == UI_HEIGHT-1) 	// Print the whole row
				if (x == 0 || x == UI_WIDTH-1)
					printf("%c ", '+');
				else
					printf("%c ", '-'); 				
			else {
				printf("%c\033[%dC%c",			// Only print the 2 sides
						'|',
						UI_WIDTH*2-3,
						'|');
				break;							
			}
		}
		printf("\n\033[%dG", AREA_WIDTH*2+2);	// New line after row
	}

	for (int i=0;i<vcount;++i)
		printf("\033[1m\033[%d;%dH%s\033[0m\033[%d;%dH%d", (i+1)*2, AREA_WIDTH*2+4, vname[i], (i+1)*2+1, AREA_WIDTH*2+4, vvalue[i]);
}

void drawSnake(int snake[SNAKE_MAX][2], int snakelen)
{
	printf("\033[1;32m");						// Color
	for (int i=0; i<snakelen; ++i) {			// For coords, print char
		if (i == snakelen-1)
			printf("\033[1;33m");
		printf("\033[%d;%dH%c", 
				snake[i][1], 
				snake[i][0]*2-1, 
				CHARS_SNAKE);
	}
	printf("\033[0m");							// Remove color
}

int isSnakeOut(int snakeHead[2]) {
	if (snakeHead[0] <= 1 ||  snakeHead[0] >= AREA_WIDTH)
		return 1;
	if (snakeHead[1] <= 1 || snakeHead[1] >= AREA_HEIGHT)
		return 1;
	return 0;
}

int isCollidingSnake(int targetX, int targetY, int snake[SNAKE_MAX][2], int snakelen) {
	for (int i=0;i<snakelen;++i)
		if (snake[i][0] == targetX && snake[i][1] == targetY)
			return 1;

	return 0;
}

int moveSnake(int dir, int snake[SNAKE_MAX][2], int snakelen, int *snakeGrowth) {
	if (!*snakeGrowth) {
		/* Add whitespace to end */
		printf("\033[%d;%dH ", 
				snake[0][1], 
				snake[0][0]*2-1);

		/* Shift array */
		for (int i=0;i<snakelen-1;++i) {
			snake[i][0] = snake[i+1][0];
			snake[i][1] = snake[i+1][1];
		}
	} else 
		--*snakeGrowth;

	/* Add head */
	switch (dir) {
		case D_RIGHT:
			snake[snakelen-1][0] = snake[snakelen-2][0] + 1;
			snake[snakelen-1][1] = snake[snakelen-2][1];
			break;
		case D_LEFT:
			snake[snakelen-1][0] = snake[snakelen-2][0] - 1;
			snake[snakelen-1][1] = snake[snakelen-2][1];
			break;
		case D_UP:
			snake[snakelen-1][0] = snake[snakelen-2][0];
			snake[snakelen-1][1] = snake[snakelen-2][1] - 1;
			break;
		case D_DOWN:
			snake[snakelen-1][0] = snake[snakelen-2][0];
			snake[snakelen-1][1] = snake[snakelen-2][1] + 1;
			break;
	}

	drawSnake(snake, snakelen);
	return 0;
}

void updateFood(int snake[SNAKE_MAX][2], int snakelen, int *foodX, int *foodY) {
	//printf("\033[%d;%dH ", *foodY, *foodX*2-1);	// Add whitespace
	while (isCollidingSnake((*foodX = rand()%(AREA_WIDTH-2)+2),
							(*foodY = rand()%(AREA_HEIGHT-2)+2), 
							snake, snakelen));
	printf("\033[1;31m");							// Color
	printf("\033[%d;%dH%c", (*foodY), 2*(*foodX)-1, CHARS_FOOD);
	printf("\033[0m");								// Remove color
}

int getSnakeColliders(int snake[SNAKE_MAX][2], int snakelen, int foodX, int foodY) {
	int snakeHeadX, snakeHeadY;
	snakeHeadX = snake[snakelen-1][0];
	snakeHeadY = snake[snakelen-1][1];

	if (isSnakeOut( snake[snakelen-1] ))
		return C_BORDER;

	if (isCollidingSnake(snakeHeadX, snakeHeadY, snake, snakelen-1))
		return C_ITSELF;

	if (isCollidingSnake(foodX, foodY, snake, snakelen))
		return C_FOOD;

	return 0;
}

int main(int argc, char const *argv[])
{
	#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
		HANDLE hconsole = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD dwMode = 0;
		GetConsoleMode(hconsole, &dwMode);
		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(hconsole, dwMode);
	#endif

	int foodX, foodY;
	int snakeGrowth = 0;
	int snake[SNAKE_MAX][2] = 					// Default snake coords
	{{AREA_WIDTH/2-2,AREA_HEIGHT/2},
	{AREA_WIDTH/2-1,AREA_HEIGHT/2},
	{AREA_WIDTH/2,AREA_HEIGHT/2}},

		snakelen = 3;							// Default snake size

	int appStartT = time(NULL);
	char *hudEntriesNames[] = {"SCORE", "TIME"};
	int  hudEntriesValues[]  = {0, 0};
	srand(time(NULL));							// Set seed of rand()
	printf("\033[2J");  						// Clear the screen
	printf("\e[?25l");							// Hides the cursor
	drawBorders();								// Draw game boundaries
	printf("Press 'q' to quit.");
	drawUI(2, hudEntriesValues,hudEntriesNames);// Draw Ui (right side)
	drawSnake(snake, snakelen); 				// Draw first state of snake
	updateFood(snake, snakelen, &foodX, &foodY);// Create first food

	int T1, T2, deltaT;							// Delta Time for consitant fps
	int state = 0x00;							// State in which the player is at
	int key;									// Last key pressed during the game
	while (state == S_ALIVE) {
		T1 = clock();
		/* Game loop */

		if (snakeGrowth > 0)
			snakelen++;

		if (kbhit())
			key = getch();

		switch (key) {
			case KEYS_UP:
				moveSnake(D_UP, snake, snakelen, &snakeGrowth);
				break;
			case KEYS_DOWN:
				moveSnake(D_DOWN, snake, snakelen, &snakeGrowth);
				break;
			case KEYS_RIGHT:
				moveSnake(D_RIGHT, snake, snakelen, &snakeGrowth);
				break;
			case KEYS_LEFT:
				moveSnake(D_LEFT, snake, snakelen, &snakeGrowth);
				break;

			case KEYS_QUIT:
				printf("\033[%dH\n", AREA_HEIGHT);	// Goto end of the Game Board
				printf("\e[?25h");					// Show the cursor
				return 0;
		}

		switch (getSnakeColliders(snake, snakelen, foodX, foodY)) {
			case C_BORDER:
			case C_ITSELF:
				state = S_DEAD;
				break;
			case C_FOOD:
				snakeGrowth += SNAKE_GROWTH;
				updateFood(snake, snakelen, &foodX, &foodY);
				break;
			default:
				
				break;
		}

		if (snakelen >= (AREA_WIDTH-2)*(AREA_HEIGHT-2)-1)
			state = S_WIN;

		/* Game Loop End */

		/* Update ScoreBoard */
		hudEntriesValues[0] = (snakelen-3+snakeGrowth)*10;
		hudEntriesValues[1] = time(NULL) - appStartT;
		drawUI(2, hudEntriesValues,hudEntriesNames);

		/* Sleep until next frame */
		T2 = clock();
		deltaT = T2 - T1;
		if (deltaT < TICK)
			SLEEP(TICK-deltaT);
		SLEEP(TICK);
	}
	hudEntriesValues[0] = (snakelen-3+snakeGrowth)*10;
	hudEntriesValues[1] = time(NULL) - appStartT;
	drawUI(2, hudEntriesValues,hudEntriesNames);// Make sure the values match up
	printf("\033[%dH\n", AREA_HEIGHT);			// Goto end of the Game Board
	printf("\e[?25h");							// Show the cursor

	switch(state) {
		case S_DEAD:
			printf("Nice try! You lasted \033[1m%lds\033[0m and went up to a length of \033[1m%d\033[0m\n", (time(NULL) - appStartT), snakelen);
			break;
		case S_WIN:
			printf("Congratulation, you finished the game in \033[1m%lds\033[0m!\n", (time(NULL) - appStartT));
			break;
	}

	printf("Press any key to quit...");
	while (!kbhit());
	getch();

	return 0;
}