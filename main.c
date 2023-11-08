// Public Domain 2023-Present.
//
// The is a free software for the public domain; you can do whatever
// to it and/or modify it.
//
// It is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

///
///	gameoflife: v0.1 Main entrypoint		<main.c>
///

#include <argp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// —————————————————————————————————————————————————————————————————————————————
// DEFINE ROW AND COLUMN COUNT OF GRID.
#define SCALE_X             1
#define SCALE_Y             1
#define NROW                24 / SCALE_X  // height visually.
#define NCOL                24 / SCALE_Y  // width visually.
#define MAX_NEIGHBOUR_COUNT 9
#define CELL_SIZE           10
#define SCREEN_WIDTH        800
#define SCREEN_HEIGHT       600

// —————————————————————————————————————————————————————————————————————————————
// DEFINE MACROS FOR TEXT COLORS
#define TEXT_COLOR_BLACK   "\x1b[30m"
#define TEXT_COLOR_RED     "\x1b[31m"
#define TEXT_COLOR_GREEN   "\x1b[32m"
#define TEXT_COLOR_YELLOW  "\x1b[33m"
#define TEXT_COLOR_BLUE    "\x1b[34m"
#define TEXT_COLOR_MAGENTA "\x1b[35m"
#define TEXT_COLOR_CYAN    "\x1b[36m"
#define TEXT_COLOR_WHITE   "\x1b[37m"
#define RESET_COLOR        "\x1b[0m"
#define TEXT_DECOR_BOLD    "\033[1m"

// —————————————————————————————————————————————————————————————————————————————
// DEFINE MACROS TO CONVERT ENUM VALUES TO STRINGS.
#define ENUM_TO_STR(enum_kind) (#enum_kind)
#define ENUM_CASE_TO_STR(enum_kind) \
    case enum_kind: return (#enum_kind)

// —————————————————————————————————————————————————————————————————————————————
// TERMINAL GLYPHS.
const char GLYPH_GRID_BORDER = ' ';  // @Disabled.
const char GLYPH_CELL_DEAD   = ' ';
const char GLYPH_CELL_ALIVE  = 'o';

// —————————————————————————————————————————————————————————————————————————————
// DATA ENUMERATIONS.
enum Game_Mode {
    GAME_GIF,
    GAME_SDL,
    GAME_TERMINAL,
};
enum Cell_Kind {
    CELL_DEAD  = 0,
    CELL_ALIVE = 1,
};
enum Color {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_DEFAULT,
};

// —————————————————————————————————————————————————————————————————————————————
// CLI ARG PARSER.
struct Arguments {
    char      *mode;
    enum Color text_color;
    int        help;
};
static struct argp_option options[] = {
    {"mode", 'm', "MODE", 0, "Set the mode (e.g., GAME_GIF, GAME_TERMINAL)"},
    {"color", 'c', "COLOR", 0, "Set text color"},
    {"help", 'h', 0, 0, "Show the hepl message"},
};
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct Arguments *args = (struct Arguments *)state->input;

    switch (key) {
    case 'm': args->mode = arg; break;
    case 'c':
        if (strcmp(arg, "red") == 0) args->text_color = COLOR_RED;
        else if (strcmp(arg, "green") == 0) args->text_color = COLOR_GREEN;
        // Add more conditional cases...
        break;
    case 'h': args->help = 1; break;
    // case ARGP_KEY_END: if (args->mode == NULL) argp_failure(state, 1, 0,
    // "Mode is required"); break;
    default: return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

// `argp` object represents the argument parser.
//
// An argp structure contains a set of:
//
// * options declarations, * a function to deal with parsing one,
// * documentation string, * a possible vector of child argp's,
// * and perhaps a function to filter help output.
static struct argp argp = {options, parse_opt, 0, 0};

// —————————————————————————————————————————————————————————————————————————————
// ERROR LOGGING.

// Print out error message and return number of chars printed to stderr.
int report_error(const char *format, ...) {
    int     result;
    va_list args;

    result = 0;
    va_start(args, format);
    result += fprintf(stderr, TEXT_DECOR_BOLD "error: " RESET_COLOR);
    result += vfprintf(stderr, format, args);
    va_end(args);

    return result;
}

// Print out a fatal message and exit with code 1.
int report_error_fatal(const char *format, ...) {
    va_list args;

    va_start(args, format);
    fprintf(stderr, TEXT_DECOR_BOLD "error: " RESET_COLOR);
    vfprintf(stderr, format, args);
    va_end(args);

    exit(1);
}

// —————————————————————————————————————————————————————————————————————————————
// TERMINAL HELPER FUNCTIONS.

void term_clear_screen() { printf("\033[2J"); }

void term_move_cursor(int row, int col) { printf("\033[%d;%dH", row, col); }

void print_grid(const int grid[NROW][NCOL]) {
    for (int i = 0; i < NROW; i++) {
        for (int j = 0; j < NCOL; j++) {
            char cell_char;

            switch (grid[i][j]) {
            case CELL_DEAD: cell_char = GLYPH_CELL_DEAD; break;
            case CELL_ALIVE: cell_char = GLYPH_CELL_ALIVE; break;
            default: report_error_fatal("unexpected cell %d", grid[i][j]);
            }

            switch (j) {
            case 0: printf("%2c ", cell_char); break;
            case (NCOL - 1): printf("%2c ", cell_char); break;
            default: printf("%2c ", cell_char); break;
            }
        }
        printf("\n");
    }
}

// —————————————————————————————————————————————————————————————————————————————
// GAME LOGIC.

// Update game of life state.
void update_state(int new_grid[NROW][NCOL], const int grid[NROW][NCOL]) {
    // —————————————————————————————————————————————————————————————————————————
    // Copy grid to new_grid.
    for (int i = 0; i < NROW; i += 1) {
        for (int j = 0; j < NCOL; j += 1) { new_grid[i][j] = grid[i][j]; }
    }
    // —————————————————————————————————————————————————————————————————————————
    // Run a single game instant.
    for (int x = 0; x < NROW; x += 1) {
        for (int y = 0; y < NCOL; y += 1) {
            // —————————————————————————————————————————————————————————————————
            // Calculate count of live offset neighbours.
            int live_neighbours = 0;

            for (int dx = -1; dx <= 1; dx += 1) {  // Collect 3x3 neighbours.
                for (int dy = -1; dy <= 1; dy += 1) {
                    int nx = x + dx, ny = y + dy;

                    if (dx == 0 && dy == 0) continue;  // Skip if it's cur cell.

                    if ((nx >= 0 && nx < NROW) && (ny >= 0 && ny < NCOL)) {
                        live_neighbours += new_grid[nx][ny];  // Inside bounds.
                    }
                }
            }
            // —————————————————————————————————————————————————————————————————
            // Apply rules of the game.
            switch (grid[x][y]) {
            case CELL_ALIVE:
                if (live_neighbours < 2 || live_neighbours > 3)
                    new_grid[x][y] = CELL_DEAD;
                else new_grid[x][y] = CELL_ALIVE;
                break;
            case CELL_DEAD:
                if (live_neighbours == 3) new_grid[x][y] = CELL_ALIVE;
                else new_grid[x][y] = CELL_DEAD;
                break;
            }
        }
    }
}

// Update game of life state for current frame's buffer and
// mutate image.
void update_buffer_and_img(void *img, int new_grid[NROW][NCOL],
                           int grid[NROW][NCOL], const int frame_num) {
    update_state(new_grid, grid);

    for (int i = 0; i < NROW; i += 1) {  // Copy updated new_grid into grid.
        for (int j = 0; j < NCOL; j += 1) { grid[i][j] = new_grid[i][j]; }
    }
};

// —————————————————————————————————————————————————————————————————————————————
// BUFFER WRITER IMAGE HELPERS.

// Write bytes to a stream or file.
void write_bytes(FILE *file, uint8_t *data, int size) {
    for (int i = 0; i < size; i += 1) fputc(data[i], file);
}

void convert_grid_to_image(int            new_grid[NROW][NCOL],
                           uint8_t *const image_data) {
    uint8_t background_color = 0x00;

    for (int i = 0; i < NROW; i += 1) {  // Init image with the background.
        for (int j = 0; j < NCOL; j += 1) {
            image_data[((i * NROW) + j)] = background_color;
        }
    }
    // Map `CELL_DEAD` and `CELL_ALIVE` to appropriate pixel values.
    for (int i = 0; i < NROW; i += 1) {
        for (int j = 0; j < NCOL; j += 1) {
            int     px                   = new_grid[i][j];
            uint8_t pixel_val            = (px == CELL_ALIVE) ? 0xFF : 0x00;
            image_data[((i * NROW) + j)] = pixel_val;
        }
    }
}

// —————————————————————————————————————————————————————————————————————————————
// GAME INITIAL MAP LEVELS.

void game_level_1(int grid[NROW][NCOL], int *choices_arr, int i, int j) {
    if (i != 0 && i % 17 == 0) grid[i][j] = choices_arr[i % 2];
    else if (j != 0 && j % 23 == 0) grid[i][j] = choices_arr[j % 2];
    else grid[i][j] = 0;
}

void game_level_2(int grid[NROW][NCOL], int *choices_arr, int i, int j) {
    if (i != 0 && i % 13 == 0) grid[i][j] = choices_arr[i % 2];
    else if (j != 0 && j % 19 == 0) grid[i][j] = choices_arr[j % 2];
    else grid[i][j] = 0;
}

void game_level_3(int grid[NROW][NCOL], int *choices_arr, int i, int j) {
    if (i != 0 && i % 21 == 0) grid[i][j] = choices_arr[i % 2];
    else if (j != 0 && j % 8 == 0) grid[i][j] = choices_arr[j % 2];
    else if (i != 0 && j != 0 && (i == j)) grid[i][j] = choices_arr[j % 2];
    else grid[i][j] = 0;
}

void game_level_4(int grid[NROW][NCOL], int *choices_arr, int i, int j) {
    if (i % 3 == 0) grid[i][j] = choices_arr[j % 2];
    else if (j % 5 == 0) grid[i][j] = choices_arr[i % 2];
    else grid[i][j] = 0;
}

void game_level_glider(int grid[NROW][NCOL], int i, int j) {
    // Create a glider pattern at position (i, j)
    if (i >= 0 && i + 2 < NROW && j >= 0 && j + 2 < NCOL) {
        grid[i][j + 1]     = 1;
        grid[i + 1][j + 2] = 1;
        grid[i + 2][j]     = 1;
        grid[i + 2][j + 1] = 1;
        grid[i + 2][j + 2] = 1;
    }
}

// —————————————————————————————————————————————————————————————————————————————
// MAIN.
int main(int argc, char **argv) {
    enum Game_Mode game_mode;

    // —————————————————————————————————————————————————————————————————————————
    // PARSE COMMAND LINE ARGS.
    struct Arguments args = {0};
    argp_parse(&argp, argc, argv, 0, 0, &args);
    if (args.help) {
        argp_help(&argp, stdout, ARGP_HELP_STD_HELP, argv[0]);
        return 0;
    }
    if (args.mode != NULL) {  // Use args.mode to set program's mode.
        if (strcmp(args.mode, "terminal") == 0) game_mode = GAME_TERMINAL;
        else if (strcmp(args.mode, "game") == 0) game_mode = GAME_SDL;
        else if (strcmp(args.mode, "gif") == 0) game_mode = GAME_GIF;
        else
            report_error_fatal("invalid mode '%s'\nAvailable modes: \n  "
                               "terminal\n  game\n  gif\n",
                               args.mode);
    } else {  // Provide a default mode or show an error message.
        report_error_fatal("mode not specified: Use --mode to set the mode\n");
    }
    if (args.text_color != COLOR_DEFAULT) {  // TODO:
        // Use args.mode to set program's mode.
    }
    // —————————————————————————————————————————————————————————————————————————
    // GRID INITIALIZE to 0.
    int choices_arr[]    = {0, 1};  // Fill grid with any of these values.
    int grid[NROW][NCOL] = {0};

    for (int i = 0; i < NROW; i += 1) {  // Initialize all cells as dead.
        for (int j = 0; j < NCOL; j += 1) { grid[i][j] = 0; }
    }
    // —————————————————————————————————————————————————————————————————————————
    // DECLARE & INITIALIZE MUTABLES.
    int   new_grid[NROW][NCOL] = {0};
    void *img                  = NULL;
    // —————————————————————————————————————————————————————————————————————————
    // LOAD GAME.
    switch (game_mode) {
    case GAME_GIF: {  // FIXME: TODO: make GIF work.
        // —————————————————————————————————————————————————————————————————————
        // Load game map.
        for (int i = 0; i < NROW; i += 1) {
            for (int j = 0; j < NCOL; j += 1)
                game_level_4(grid, choices_arr, i, j);
        }
        // —————————————————————————————————————————————————————————————————————
        // Load and Setup GIF file.
        const int total_frames = 60;
        char     *out          = "output.gif";  // Setup GIF file for writing.
        FILE     *gif_file     = fopen(out, "wb");
        if (!gif_file) {
            perror("Error opening file to write to\n");
            return 1;
        }
        uint8_t header[]     = "GIF89a";  // Start of GIF.
        uint8_t frame[]      = {0x2C, 0x00, 0x00, 0x00, 0x00, 0x64, 0x64, 0x00};
        const uint16_t w_gif = NROW, h_gif = NCOL;  // gif width and height.
        uint8_t        screen_desc[] = {(w_gif & 0xFF), ((w_gif >> 8) & 0xFF),
                                        (h_gif & 0xFF), ((h_gif >> 8) & 0xFF),
                                        (0xF7),         (0x00)};  // descriptor.
        uint8_t        gce[]         = {0x21, 0xF9, 0x04, 0x00, 0x0A, 0x00};
        uint8_t        trailer[]     = {0x3B};  // End of GIF.
        write_bytes(gif_file, header, 6);
        write_bytes(gif_file, screen_desc, 7);
        // —————————————————————————————————————————————————————————————————————
        // Render frames to GIF file.
        for (int frame_num = 1; frame_num <= total_frames; frame_num += 1) {
            update_buffer_and_img(img, new_grid, grid, frame_num);
            write_bytes(gif_file, frame, 9);
            uint8_t image_data[NROW * NCOL] = {0};
            convert_grid_to_image(grid, image_data);
            write_bytes(gif_file, image_data, sizeof(image_data));
            if (0) write_bytes(gif_file, gce, 7);
        }
        write_bytes(gif_file, trailer, 1);
        fclose(gif_file);  // Cleanup.
        break;
    }
    case GAME_SDL: {  // TODO:
        // —————————————————————————————————————————————————————————————————————
        // Load game map.
        for (int i = 0; i < NROW; i += 1) {
            for (int j = 0; j < NCOL; j += 1)
                game_level_3(grid, choices_arr, i, j);
        }
        // —————————————————————————————————————————————————————————————————————
        // Setup SDL.
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            report_error("could not initialize SDL2: %s\n", SDL_GetError());
            return 1;
        }
        if (TTF_Init() != 0) {
            report_error("could not initialize SDL2_ttf: %s\n", TTF_GetError());
            SDL_Quit();
            return 1;
        }
        SDL_Window   *window;
        SDL_Renderer *renderer;
        TTF_Font     *font;
        window = SDL_CreateWindow("Game of Life", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH,
                                  SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (window == NULL) {
            SDL_Quit();
            report_error_fatal("%s\n", SDL_GetError());
        }
        renderer = SDL_CreateRenderer(
            window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        if (renderer == NULL) {
            SDL_DestroyWindow(window);
            SDL_Quit();
            report_error_fatal("%s\n", SDL_GetError());
        };
        // —————————————————————————————————————————————————————————————————————
        // Main game loop.
        const int n_frames = 60, delay_ms = 100;

        while (1) {
            SDL_Event event;
            if (SDL_PollEvent(&event)) {  // Handle events.
                if (event.type == SDL_QUIT) break;
            }
            // —————————————————————————————————————————————————————————————————
            // Game logic and rendering here.
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // bg color.
            SDL_RenderClear(renderer);  // Clear cur target with the bg color.
            // Update game state.
            for (int frame_num = 1; frame_num <= n_frames; frame_num += 1) {
                update_buffer_and_img(img, new_grid, grid, frame_num);
            }
            // Set the updated game state to renderer.
            for (int x = 0; x < NROW; x += 1) {
                for (int y = 0; y < NCOL; y += 1) {
                    SDL_Rect cell_rect = {x * CELL_SIZE, y * CELL_SIZE,
                                          CELL_SIZE, CELL_SIZE};
                    switch (grid[x][y]) {
                    case CELL_ALIVE:
                        SDL_SetRenderDrawColor(renderer, 170, 170, 0, 255);
                        break;
                    case CELL_DEAD:
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                        break;
                    }
                    SDL_RenderFillRect(renderer, &cell_rect);
                }
            }
            // —————————————————————————————————————————————————————————————————
            // Update screen with any rendering performed since previous call.
            SDL_RenderPresent(renderer);
            SDL_Delay(delay_ms);
        }  // while (1)
        // —————————————————————————————————————————————————————————————————————
        // Cleanup and exit.
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        break;
    }
    case GAME_TERMINAL: {
        // —————————————————————————————————————————————————————————————————————
        // Load game map.
        for (int i = 0; i < NROW; i += 1) {
            for (int j = 0; j < NCOL; j += 1)
                game_level_3(grid, choices_arr, i, j);
        }
        // —————————————————————————————————————————————————————————————————————
        // Setup terminal animation.
        int        fps = (int)(120 / 10), animate_dur_secs = (int)(10 * 2.5);
        double     n_frames           = animate_dur_secs * fps;
        double     interval_frames_ms = (animate_dur_secs * 1000) / n_frames;
        float      interval_frames_s  = interval_frames_ms / 1000;
        useconds_t interval_frames_microsecond = interval_frames_ms * 1000;
        // —————————————————————————————————————————————————————————————————————
        // Update state and render each frame.
        for (int frame_num = 1; frame_num <= n_frames; frame_num += 1) {
            usleep(interval_frames_microsecond);
            term_clear_screen();
            term_move_cursor(1, 1);
            {  // Show animation dev information.
                printf("%s", TEXT_COLOR_GREEN);
                printf("delay(s)       %7.2f\n", interval_frames_s);
                printf("duration(s)    %4d \n", animate_dur_secs);
                printf("frames/sec     %4d\n", fps);
                printf("frame          %4d/%d\n", frame_num, (int)n_frames);
                printf("%s\n", RESET_COLOR);
            }
            update_buffer_and_img(img, new_grid, grid, frame_num);
            printf("%s", TEXT_COLOR_YELLOW);
            print_grid(new_grid);
            printf("%s\n", RESET_COLOR);
        }
        break;
    }
    default: report_error_fatal("unexpected game mode %d", game_mode);
    }
    return 0;
}
