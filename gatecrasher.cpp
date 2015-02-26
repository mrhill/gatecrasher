// gcc -o gatecrasher -std=c++11 gatecrasher.cpp gc.cpp -lstdc++ -lncurses -g -O0

#include "gc.h"
#include <curses.h>
#include <signal.h>
#include <unistd.h>
#include <cstdlib>

static void finish(int sig)
{
    endwin();
    exit(0);
}

static void render(IGame& game)
{
	IField& field = game.field();
	int x, y;

	attrset(COLOR_PAIR(5));
	for (y = 0; y < field.mHeight; y++) {
		move(y, 0);
		for (x = 0; x < field.mWidth; x++) {
			uint8_t t = field.mpMap[y*field.mWidth + x];
			if ((t >> TILETYPESHIFT) == TILETYPE_WALL)
				addch("T_[]/\\\\/][W"[t & TILEMASK]);
			else
				addch(" W/u"[t >> TILETYPESHIFT]);
		}
	}

	attrset(COLOR_PAIR(7));
	mvprintw(0,0, "P:%06d", game.mPoints);
	mvprintw(1,0, "L:%02d", game.mLevel);

	Car& car = game.car();
	attrset(COLOR_PAIR(1));
	mvaddstr(car.pos().y-1, car.pos().x-2, "__|__");
	mvaddstr(car.pos().y,   car.pos().x-2, "<[ ]>");

	attrset(COLOR_PAIR(3));
	for (uint32_t i = game.ballCount()-1, x=field.mWidth; (int)i>=0; i--) {
		Ball& ball = game.ball(i);
		if (ball.mState == BALLSTATE_FRESH)
			mvaddch(0, --x, 'o');
		else if (ball.mState != BALLSTATE_BUST)
			mvaddch(ball.mPos.y, ball.mPos.x, 'o');
	}
}

int main(int argc, char** argv)
{
	IGame& game = IGame::instance();

	signal(SIGINT, finish);

	auto w = initscr();     /* initialize the curses library */
    keypad(stdscr, TRUE);	/* enable keyboard mapping */
    nonl();         		/* tell curses not to do NL->CR/NL on output */
    cbreak();       		/* take input chars one at a time, no wait for \n */
    nodelay(w, TRUE);		/* non-blocking keyboard input */
    noecho();       		/* no echo input */
    curs_set(0);			/* cursor off */

    if (has_colors())
    {
        start_color();
        init_pair(1, COLOR_RED,     COLOR_BLACK);
        init_pair(2, COLOR_GREEN,   COLOR_BLACK);
        init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
        init_pair(4, COLOR_BLUE,    COLOR_BLACK);
        init_pair(5, COLOR_CYAN,    COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_WHITE,   COLOR_BLACK);
    }

	game.initLevel(0);

    for(;;) {
    	uint32_t keymask = GCKEY_NONE;
	    switch (getch()) {
	    	case KEY_LEFT:  case 'h': keymask = GCKEY_LEFT; break;
	    	case KEY_RIGHT: case 'l': keymask = GCKEY_RIGHT; break;
	    	case KEY_UP:    case 'k': keymask = GCKEY_UP; break;
	    	case KEY_DOWN:  case 'j': keymask = GCKEY_DOWN; break;
	    	case KEY_ENTER: case ' ': keymask = GCKEY_FIRE; break;
	    }
    	game.poll(keymask);
	    render(game);
	    usleep(1000000/60);
	}

	finish(0);
	return 0;
}
