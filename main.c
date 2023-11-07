#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define N 48
// #define MOVES_4DIR 4

enum Cell_Kind {
    CELL_DEAD  = 0,
    CELL_ALIVE = 1,
};

enum Game_Mode {
    GAME_GIF,
    GAME_TERMINAL,
};

// struct Coordinates { int x; int y; };
// const struct Coordinates Start_Coord = {2, 2};
// const struct Coordinates End_Coord   = {(N - 3), (N - 3)};
// const struct Coordinates Offsets_Dx_Dy[MOVES_4DIR] = {
//   {-1, 0}, {1, 0}, {0, -1}, {0, 1}};

char CHAR_FMT_GRID_BORDER = '.';
char CHAR_FMT_CELL_DEAD   = ' ';
char CHAR_FMT_CELL_ALIVE  = '*';

// const char Directions[MOVES_4DIR] = {'U', 'D', 'L', 'R'};

// int is_start(int x, int y){return x==Start_Coord.x && y==Start_Coord.y; }
// int is_end(int x, int y) { return x == End_Coord.x && y == End_Coord.y; }

void print_horizontal_line(int repeat, const char pattern) {
    printf(" ");  // corner.
    for (int i = 0; i < repeat; i++) printf("%c", pattern);
    printf(" ");  // corner.
    printf("\n");
};

void print_grid(const int grid[N][N]) {
    int gap = 3;  // '%2c ' (2+1) cell_char space.

    print_horizontal_line((N * gap), CHAR_FMT_GRID_BORDER);

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            char cell_char;

            switch (grid[i][j]) {
            case CELL_DEAD: cell_char = CHAR_FMT_CELL_DEAD; break;
            case CELL_ALIVE: cell_char = CHAR_FMT_CELL_ALIVE; break;
            default:
                fprintf(stderr, "error: invalid maze cell: %i\n", grid[i][j]);
                exit(1);
            }

            if (j == 0) printf("%c%2c ", CHAR_FMT_GRID_BORDER, cell_char);
            else if (j == (N - 1))
                printf("%2c %c", cell_char, CHAR_FMT_GRID_BORDER);
            else printf("%2c ", cell_char);
        }
        printf("\n");
    }
    print_horizontal_line((N * gap), CHAR_FMT_GRID_BORDER);
}

// Update game of life state.
void update_state(int new_grid[N][N], const int grid[N][N]) {
    // —————————————————————————————————————————————————————————————————————————
    // Copy grid to new_grid.
    for (int i = 0; i < N; i += 1) {
        for (int j = 0; j < N; j += 1) { new_grid[i][j] = grid[i][j]; }
    }
    // —————————————————————————————————————————————————————————————————————————
    // Run a single game instant.
    for (int x = 0; x < N; x += 1) {
        for (int y = 0; y < N; y += 1) {
            // —————————————————————————————————————————————————————————————————
            // Collect 3x3 matrix offset neighbours.
            int neighbours[N * N] = {0}, n_neigh = 0;
            for (int dx = -1; dx <= 1; dx += 1) {  // row offset.
                int row[3] = {0}, n_row = 0;
                for (int dy = -1; dy <= 1; dy += 1)  // column offset.
                    row[n_row++] = new_grid[x + dx][y + dy];
                for (int i = 0; i < 3; i += 1) neighbours[n_neigh++] = row[i];
            }
            // —————————————————————————————————————————————————————————————————
            // Calculate count of live offset neighbours.
            int live_neigh = 0, neigh_sum = 0;  // Get sum of neighbours.
            for (int i = 0; i < n_neigh; i += 1) neigh_sum += neighbours[i];
            live_neigh = neigh_sum - grid[x][y];

            // —————————————————————————————————————————————————————————————————
            // Apply rules of the game.
            switch (grid[x][y]) {
            case CELL_ALIVE:
                if (live_neigh < 2 || live_neigh > 3)
                    new_grid[x][y] = CELL_DEAD;
                break;
            case CELL_DEAD:
                if (live_neigh == 3) new_grid[x][y] = CELL_ALIVE;
                break;
            default:
                fprintf(stderr, "error: Found unexpected cell %d", grid[x][y]);
                exit(1);
            }
        }
    }
}

// Update game of life state for current frame's buffer and mutate image.
void update_buffer_and_img(void *img, int new_grid[N][N], int grid[N][N],
                           const int frame_num) {
    update_state(new_grid, grid);
    // —————————————————————————————————————————————————————————————————————————
    // Copy updated new_grid into grid.
    for (int i = 0; i < N; i += 1) {
        for (int j = 0; j < N; j += 1) { grid[i][j] = new_grid[i][j]; }
    }
};

void term_clear_screen() { printf("\033[2J"); }

void term_move_cursor(int row, int col) { printf("\033[%d;%dH", row, col); }

// Write bytes to a stream or file.
void write_bytes(FILE *file, uint8_t *data, int size) {
    for (int i = 0; i < size; i += 1) fputc(data[i], file);
}

void convert_grid_to_image(int new_grid[N][N], uint8_t *const image_data) {
    uint8_t background_color = 0x00;
    // —————————————————————————————————————————————————————————————————————————
    // Initialize image with the background color..
    for (int i = 0; i < N; i += 1) {
        for (int j = 0; j < N; j += 1) {
            image_data[((i * N) + j)] = background_color;
        }
    }
    // —————————————————————————————————————————————————————————————————————————
    // Map `CELL_DEAD` and `CELL_ALIVE` to appropriate pixel values.
    for (int i = 0; i < N; i += 1) {
        for (int j = 0; j < N; j += 1) {
            int     px                = new_grid[i][j];
            uint8_t pixel_val         = (px == CELL_ALIVE) ? 0xFF : 0x00;
            image_data[((i * N) + j)] = pixel_val;
        }
    }
}

int main(void) {
    enum Game_Mode game_mode;

    game_mode = GAME_GIF;
    game_mode = GAME_TERMINAL;
    // —————————————————————————————————————————————————————————————————————————
    // Setup file for writing.
    char *out      = "output.gif";
    FILE *gif_file = fopen(out, "wb");
    if (!gif_file) {
        perror("Error opening file to write to\n");
        return 1;
    }
    // —————————————————————————————————————————————————————————————————————————
    // Write GIF header.
    uint8_t gif_header[] = "GIF89a";
    write_bytes(gif_file, gif_header, 6);
    // —————————————————————————————————————————————————————————————————————————
    // Logical Screen Descriptor.
    uint16_t width = N, height = N;
    uint8_t  screen_descriptor[] = {
        (width & 0xFF),  ((width >> 8) & 0xFF),
        (height & 0xFF), ((height >> 8) & 0xFF),
        (0xF7),  // Packed field (global color table, 8-bit color resolution)
        (0x00),  // Background color index.
    };
    write_bytes(gif_file, screen_descriptor, 7);
    // —————————————————————————————————————————————————————————————————————————
    // Populate initial grid.
    int choices_arr[] = {0, 1};  // Fill grid with any of these values.
    int grid[N][N]    = {0};
    for (int i = 0; i < N; i += 1) {
        for (int j = 0; j < N; j += 1) {
            if (i % 7 == 0) grid[i][j] = choices_arr[j % 2];
            else if (j % 13 == 0) grid[i][j] = choices_arr[i % 2];
            else grid[i][j] = 0;
        }
    }

    int   new_grid[N][N] = {0};
    void *img            = NULL;
    // —————————————————————————————————————————————————————————————————————————
    // Start Game of Life.
    switch (game_mode) {
    // FIXME: make it work.
    case GAME_GIF: {
        int num_frames = 30;
        for (int frame_num = 1; frame_num <= num_frames; frame_num += 1) {
            if (0) {
                term_clear_screen();
                term_move_cursor(1, 1);
                {  // Print animation information.
                    printf("frame          %4d/%d\n", frame_num,
                           (int)num_frames);
                }
                update_buffer_and_img(img, new_grid, grid, frame_num);
                print_grid(new_grid);
                usleep(1000000);  // in microseconds.
            } else {
                // —————————————————————————————————————————————————————————————————————————
                // Update state and render frame.
                update_buffer_and_img(img, new_grid, grid, frame_num);
                // —————————————————————————————————————————————————————————————————————————
                // Frame.
                uint8_t frame[] = {
                    0x2C,                    // Image descriptor.
                    0x00, 0x00, 0x00, 0x00,  // Image position&size.
                    0x64, 0x64,              // Width and height(100x100).
                    0x00  // Packed field (no local color table, interlace, or
                          // disposal)
                };
                write_bytes(gif_file, frame, 9);
                // —————————————————————————————————————————————————————————————————————————
                // Image Data for the frame (capture the current state of the
                // grid.) Populate image_data with current grid state
                uint8_t image_data[N * N] = {0};
                convert_grid_to_image(grid, image_data);
                for (int i = 0, n = N * N; i < n; i += 1) {
                    // __AUTO_GENERATED_PRINT_VAR_START__
                    printf("%3hhu ",
                           image_data[i]);  // __AUTO_GENERATED_PRINT_VAR_END__
                    if ((i + 1) % N == 0) { printf("\n"); }
                }
                // __AUTO_GENERATED_PRINTF_START__
                printf("main#for#if 1(%d): \n",
                       __LINE__);  // __AUTO_GENERATED_PRINTF_END__
                write_bytes(gif_file, image_data, sizeof(image_data));
                if (0) {  // GCE isn't working for now.
                    // —————————————————————————————————————————————————————————————————————————
                    // Graphics Control Extension (GCE) for frame duration.
                    uint8_t gce[] = {
                        0x21, 0xF9, 0x04,  // GCE block start and size.
                        0x00, 0x0A,  // Packed field & delay time (10 hundredths
                                     // of a second.)
                        0x00         // Transparent color index (not used here.)
                    };
                    write_bytes(gif_file, gce, 7);
                } else {
                    usleep(100000);
                }
            }
        }
        break;
    }
    case GAME_TERMINAL: {
        int        desired_fps = 60, animate_dur_secs = 30;
        double     n_frames           = animate_dur_secs * desired_fps;
        double     interval_frames_ms = (animate_dur_secs * 1000) / n_frames;
        float      interval_frames_s  = interval_frames_ms / 1000;
        useconds_t interval_frames_micros = interval_frames_ms * 1000;
        // —————————————————————————————————————————————————————————————————————
        // Render animation frame by frame.
        for (int frame_num = 1; frame_num <= n_frames; frame_num += 1) {
            term_clear_screen();
            term_move_cursor(1, 1);
            {  // Print animation information.
                printf("delay(s)       %7.2f\n", interval_frames_s);
                printf("duration(s)    %4d \n", animate_dur_secs);
                printf("frames/sec     %4d\n", desired_fps);
                printf("frame          %4d/%d\n", frame_num, (int)n_frames);
            }
            update_buffer_and_img(img, new_grid, grid, frame_num);
            print_grid(new_grid);
            usleep(interval_frames_micros);  // in microseconds.
        }
        break;
    }
    default:
        fprintf(stderr, "error: unexpected game mode %d", game_mode);
        exit(1);
    }
    // —————————————————————————————————————————————————————————————————————————
    // Write GIF trailer.
    uint8_t trailer[] = {0x3B};  // End of GIF.
    write_bytes(gif_file, trailer, 1);
    // —————————————————————————————————————————————————————————————————————————
    // Cleanup.
    fclose(gif_file);

    return 0;
}
