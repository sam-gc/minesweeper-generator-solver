#include <stdio.h>
#include "solver.h"

int main(int argc, char *argv[])
{
    int w = 30, h = 16, mn = 99;
    if(argc > 1)
    {
        w = atoi(argv[1]);
        h = atoi(argv[2]);
        mn = atoi(argv[3]);
    }

    set_constraints(w, h, mn);
    tile_t **board = gen_no_guess_board(0, 0);
    free_board(board);
    return 0;
}
