#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "ant.h"
#include "raylib.h"

typedef struct {
    int win_width;
    int win_height;
    int board_cell_size;
    int frames_count;
    int step;
    int steps_per_frame;
    int target_fps;
    bool paused;
    bool show_info;
    bool show_grid;
    bool center_camera;
    bool single_step;
    double elapsed_secs;
    // fps updated once per second
    double fps;
    // total sum of fps.that were
    // computed each frame
    double fps_sum;
    struct timespec prev_frame_timestamp, actual_frame_timestamp;
} ant_sim_config;

/*
===============
draw_info

Draw information panel on screen if config->show_info is true.
Panel id drawn on blue rectangle.
===============
*/
void draw_info(int step, int steps_per_frame, ant_vec *board_size, ant_sim_config *config) {
    char buf[512];
    sprintf(buf,
            "SPF: %d\n"
            "FPS: %.2lf\n"
            "STEP: %d\n"
            "H: HIDE INFO\n"
            "SPACE: PAUSE\n"
            "S: STEP\n"
            "C: CENTER ANT\n"
            "+: INCREASE SPF\n"
            "-: DECREASE SPF\n"
            "LEFT/RIGHT: MOVE CAMERA HORIZONTAL\n"
            "UP/DOWN: MOVE CAMERA VERTICAL\n"
            "]: ZOOM IN\n"
            "[: ZOOM OUT\n"
            "SPF: STEPS PER FRAME\n"
            "CELL SIZE: %d\n"
            "BOARD SIZE: (ROWS: %d, COLS: %d)\n"
            "ESC: CLOSE APP\n"
            "LIB VER: %s",
            steps_per_frame, config->fps, step, config->board_cell_size,
            board_size->row, board_size->col, ant_str_version());
    Vector2 text_size = MeasureTextEx(GetFontDefault(), buf, 10, 1.0f);
    DrawRectangle(5, 5, (int)text_size.x + 5, (int)text_size.y + 5, BLUE);
    DrawText(buf, 10, 10, 10, BLACK);
}

/*
===============
handle_keyboard

Handles keys press and modifies config
or camera properties.
===============
*/
void handle_keyboard(ant_sim_config *config, Camera2D *camera) {
    int camera_target_delta = 10;
    double camera_zoom_delta = .1;
    double camera_zoom_min = .2;
    double camera_zoom_max = 3.;

    if (IsKeyPressed(KEY_SPACE))
        config->paused = !config->paused;
    if (IsKeyPressed(KEY_H))
        config->show_info = !config->show_info;
    if (IsKeyPressed(KEY_C))
        config->center_camera = !config->center_camera;
    if (IsKeyPressed(KEY_G))
        config->show_grid = !config->show_grid;
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
        if (config->steps_per_frame > 1)
            config->steps_per_frame--;
    }
    if (IsKeyPressed(KEY_KP_ADD) || 
        (IsKeyPressed(KEY_EQUAL) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))) {
        if (config->steps_per_frame < 100)
            config->steps_per_frame++;
    }
    if (IsKeyPressed(KEY_S))
        config->single_step = true;
    if (IsKeyDown(KEY_LEFT))
        camera->target.x -= camera_target_delta;
    if (IsKeyDown(KEY_RIGHT))
        camera->target.x += camera_target_delta;
    if (IsKeyDown(KEY_UP))
        camera->target.y -= camera_target_delta;
    if (IsKeyDown(KEY_DOWN))
        camera->target.y += camera_target_delta;
    if (IsKeyDown(KEY_LEFT_BRACKET)) {
        camera->zoom -= camera_zoom_delta;
        if (camera->zoom < camera_zoom_min)
            camera->zoom = camera_zoom_min;
    }
    if (IsKeyDown(KEY_RIGHT_BRACKET)) {
        camera->zoom += camera_zoom_delta;
        if (camera->zoom > camera_zoom_max)
            camera->zoom = camera_zoom_max;
    }
}

/*
===============
draw_grid_lines
===============
*/
void draw_grid_lines(ant_vec board_size, int cell_size, ant_sim_config *config) {
    for (int r=0; r <= board_size.row; r++) {
        int y = (config->win_height - board_size.row * cell_size) / 2 + r * cell_size;
        int x_min = (config->win_width - board_size.col * cell_size) / 2;
        int x_max = (config->win_width - board_size.col * cell_size) / 2 + board_size.col * cell_size;
        DrawLine(x_min, y, x_max, y, BLACK);
    }
    for (int c=0; c <= board_size.col; c++) {
        int x = (config->win_width - board_size.col * cell_size) / 2 + c * cell_size;
        int y_min = (config->win_height - board_size.row * cell_size) / 2;
        int y_max = (config->win_height - board_size.row * cell_size) / 2 + board_size.row * cell_size;
        DrawLine(x, y_min, x, y_max, BLACK);
    }
}

ant_world* read_input(ant_sim_config *config) {
    int rows, cols, num_ants;
    char row[100];
    scanf("%d", &config->win_width);
    scanf("%d", &config->win_height);
    scanf("%d", &config->target_fps);
    scanf("%d", &config->board_cell_size);
    scanf("%d %d", &rows, &cols);
    scanf("%d", &num_ants);
    ant_world *world = ant_init((ant_vec){rows, cols});
    ant_vec ant_pos;
    ant_direction ant_dir;
    for (int i = 0; i < num_ants; i++) {
        scanf("%d %d %d", &ant_pos.row, &ant_pos.col, (int*)&ant_dir);
        ant_add_ant(world, ant_pos, ant_dir);
    }
    for (int r = 0; r < rows; r++) {
        scanf("%s", row);
        printf("%s\n", row);
        for (int c = 0; c < cols; c++)
            if (row[c] == 'B')
                ant_set_board_cell(world, (ant_vec){r, c}, ANT_BOARD_CELL_BLACK);
    }
    return world;
}

void update_fps(ant_sim_config *config) {
    clock_gettime(CLOCK_MONOTONIC, &config->actual_frame_timestamp);
    double sec_since_last_frame = (double)(config->actual_frame_timestamp.tv_sec - config->prev_frame_timestamp.tv_sec) +
                                  (double)(config->actual_frame_timestamp.tv_nsec - config->prev_frame_timestamp.tv_nsec) / 1000000000.;
    int full_secs_prev = (int)config->elapsed_secs;
    config->elapsed_secs += sec_since_last_frame;
    config->prev_frame_timestamp = config->actual_frame_timestamp;
    config->fps_sum += 1 / sec_since_last_frame;
    config->frames_count++;
    if ((int)config->elapsed_secs > full_secs_prev) {
        config->fps = 1. / sec_since_last_frame;
    }
}

void draw_board(ant_sim_config *config, ant_world *world, Texture2D *ant_tex) {
    for (int r=0; r < world->board.board_size.row; r++) {
        if (config->show_grid)
            draw_grid_lines(world->board.board_size, config->board_cell_size, config);
        for (int c=0; c < world->board.board_size.col; c++) {
            int pos_x = (config->win_width - world->board.board_size.col * config->board_cell_size) / 2 + c * config->board_cell_size;
            int pos_y = (config->win_height - world->board.board_size.row * config->board_cell_size) / 2 + r * config->board_cell_size;
            if (world->board.board[r * world->board.board_size.col + c] == ANT_BOARD_CELL_BLACK)
                DrawRectangle(pos_x, pos_y, config->board_cell_size, config->board_cell_size, BLACK);
            for (int a = 0; a < world->num_ants; a++) {
                if (world->ants[a].pos.row == r && world->ants[a].pos.col == c) {
                    float angle = (float)(90 * world->ants[a].dir);
                    DrawTexturePro(*ant_tex,
                                   (Rectangle){0, 0, (float)ant_tex->width, (float)ant_tex->height},
                                   (Rectangle){(float)(pos_x + config->board_cell_size / 2), (float)(pos_y + config->board_cell_size / 2), (float)config->board_cell_size, (float)config->board_cell_size},
                                   (Vector2){(float)(config->board_cell_size / 2), (float)(config->board_cell_size / 2) },
                                   angle,
                                   WHITE);
                }
            }
        }
    }
}

int main(void) {
    ant_sim_config config = {
        .paused = false,
        .show_info = true,
        .elapsed_secs = .0,
        .frames_count = 0,
        .fps_sum = .0,
        .show_grid = true,
        .center_camera = true,
        .step = 0,
        .steps_per_frame = 1,
        .single_step = false
    };
    ant_world *world;
    world = read_input(&config);
    clock_gettime(CLOCK_MONOTONIC, &config.prev_frame_timestamp);

    InitWindow(config.win_width, config.win_height, "Langton's Ant");

    Camera2D camera = {0};
    camera.offset = (Vector2){(float)(config.win_width/2), (float)(config.win_height/2)};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    Texture2D ant_tex = LoadTexture("../img/ant.png");

    SetTargetFPS(config.target_fps);
    while (!WindowShouldClose()) {
        BeginDrawing();
        handle_keyboard(&config, &camera);
        if (config.center_camera) {
            camera.target.x = (float)(config.win_width / 2 - world->board.board_size.col * config.board_cell_size / 2 + world->ants[0].pos.col * config.board_cell_size + config.board_cell_size / 2);
            camera.target.y = (float)(config.win_height / 2 - world->board.board_size.row * config.board_cell_size / 2 + world->ants[0].pos.row * config.board_cell_size + config.board_cell_size / 2);
            config.center_camera = false;
        }
        ClearBackground(BLUE);

        BeginMode2D(camera);
        draw_board(&config, world, &ant_tex);
        EndMode2D();

        update_fps(&config);
        if (config.show_info)
            draw_info(config.step, config.steps_per_frame, &world->board.board_size, &config);
        if (!config.paused) {
            for (int s = 0; s < config.steps_per_frame; s++) {
                ant_step(world);
                config.step++;
            }
        }
        if (config.single_step) {
            ant_step(world);
            config.step++;
            config.single_step = false;
        }

        EndDrawing();
    }
    ant_free(world);
    printf("AVG FPS: %.2lf\n", ((double)config.fps_sum) / config.frames_count);
    CloseWindow();

    return 0;
}