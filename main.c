#include <stdio.h>
#include "solver.h"

int main(int argc, char *argv[])
{
    set_constraints(30, 16, 99);
    tile_t **board = gen_no_guess_board(0, 0);
    free_board(board);
    return 0;
}