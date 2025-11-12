#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <time.h>
#include <locale.h>

#define WIDTH 30
#define HEIGHT 15
#define MAX_SNAKE 100

typedef enum { UP, DOWN, LEFT, RIGHT } Direction;

typedef struct {
    int x[MAX_SNAKE];
    int y[MAX_SNAKE];
    int length;
    Direction dir;
} Snake;

typedef struct {
    int x, y;
} Food;

struct termios orig_termios;

// ===== Terminal Helpers =====
void disableRawMode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int kbhit(void) {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

int read_key() {
    unsigned char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return 0;
    if (seq[0] != '\033') return seq[0];

    // Might be an escape sequence
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\033';
    if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\033';

    if (seq[1] == '[') {
        switch (seq[2]) {
            case 'A': return 'U'; // Up
            case 'B': return 'D'; // Down
            case 'C': return 'R'; // Right
            case 'D': return 'L'; // Left
        }
    }
    return 0;
}

// ===== Game Drawing =====
void draw(Snake *snake, Food *food) {
    printf("\033[H"); // move cursor to top-left

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            // Borders
            if (y == 0 && x == 0) { printf("┌"); continue; }
            if (y == 0 && x == WIDTH - 1) { printf("┐"); continue; }
            if (y == HEIGHT - 1 && x == 0) { printf("└"); continue; }
            if (y == HEIGHT - 1 && x == WIDTH - 1) { printf("┘"); continue; }
            if (y == 0 || y == HEIGHT - 1) { printf("─"); continue; }
            if (x == 0 || x == WIDTH - 1) { printf("│"); continue; }

            // Food
            if (x == food->x && y == food->y) {
                printf("\033[31m●\033[0m");
                continue;
            }

            // Snake
            int printed = 0;
            for (int i = 0; i < snake->length; i++) {
                if (snake->x[i] == x && snake->y[i] == y) {
                    if (i == 0)
                        printf("\033[32m█\033[0m"); // head
                    else
                        printf("\033[92m▓\033[0m"); // body
                    printed = 1;
                    break;
                }
            }
            if (!printed) printf(" ");
        }
        printf("\n");
    }

    printf("Score: %d\n", snake->length - 1);
}

// ===== Game Logic =====
void place_food(Snake *snake, Food *food) {
    int valid = 0;
    while (!valid) {
        valid = 1;
        food->x = rand() % (WIDTH - 2) + 1;
        food->y = rand() % (HEIGHT - 2) + 1;
        for (int i = 0; i < snake->length; i++) {
            if (snake->x[i] == food->x && snake->y[i] == food->y)
                valid = 0;
        }
    }
}

int collision(Snake *snake) {
    if (snake->x[0] <= 0 || snake->x[0] >= WIDTH - 1 ||
        snake->y[0] <= 0 || snake->y[0] >= HEIGHT - 1)
        return 1;
    for (int i = 1; i < snake->length; i++)
        if (snake->x[0] == snake->x[i] && snake->y[0] == snake->y[i])
            return 1;
    return 0;
}

void update(Snake *snake, Food *food) {
    for (int i = snake->length - 1; i > 0; i--) {
        snake->x[i] = snake->x[i - 1];
        snake->y[i] = snake->y[i - 1];
    }

    switch (snake->dir) {
        case UP: snake->y[0]--; break;
        case DOWN: snake->y[0]++; break;
        case LEFT: snake->x[0]--; break;
        case RIGHT: snake->x[0]++; break;
    }

    if (snake->x[0] == food->x && snake->y[0] == food->y) {
        if (snake->length < MAX_SNAKE) snake->length++;
        place_food(snake, food);
    }
}

// ===== Input =====
void handle_input(Snake *snake) {
    if (!kbhit()) return;

    int key = read_key();

    switch (key) {
        case 'U': if (snake->dir != DOWN) snake->dir = UP; break;
        case 'D': if (snake->dir != UP) snake->dir = DOWN; break;
        case 'L': if (snake->dir != RIGHT) snake->dir = LEFT; break;
        case 'R': if (snake->dir != LEFT) snake->dir = RIGHT; break;
        case 'q': case 'Q':
            printf("\033[?25h\033[H\033[2JGoodbye!\n");
            disableRawMode();
            exit(0);
    }
}

// ===== Main =====
int main(void) {
    setlocale(LC_ALL, "");
    srand(time(NULL));
    enableRawMode();

    printf("\033[2J");   // Clear screen
    printf("\033[?25l"); // Hide cursor

    Snake snake = {{WIDTH / 2}, {HEIGHT / 2}, 3, RIGHT};
    for (int i = 1; i < snake.length; i++) {
        snake.x[i] = snake.x[0] - i;
        snake.y[i] = snake.y[0];
    }

    Food food;
    place_food(&snake, &food);

    while (1) {
        draw(&snake, &food);
        handle_input(&snake);
        update(&snake, &food);

        if (collision(&snake)) {
            printf("\033[H\033[2JGame Over! Final score: %d\n", snake.length - 1);
            break;
        }

        int speed = 180000 - (snake.length * 4000);
        if (speed < 40000) speed = 40000;
        usleep(speed);
    }

    printf("\033[?25h"); // Show cursor again
    disableRawMode();
    return 0;
}

