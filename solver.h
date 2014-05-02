#ifndef SOLVER_H
#define SOLVER_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct tile {
    char x;
    char y;
    char nm_count;
    char flagged;
    char mine;
    char cleared;
    struct tile *neighbors[8];
} tile_t;

void set_constraints(int w, int h, int mines);
tile_t **gen_random_board(int start_x, int start_y);
tile_t **gen_no_guess_board(int start_x, int start_y);
char solve_board(tile_t **board);
void free_board(tile_t **board);

#ifdef __cplusplus
}
#endif

#endif