#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "solver.h"

#define GEN_MAX 2000

static int board_width;
static int board_height;
static int board_mines;

static int offsets[8][2] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};

typedef struct node {
    struct node *next;
    tile_t *tile;
} node_t;

typedef struct {
    node_t *head;
} list_t;

list_t empty_list()
{
    list_t l = {NULL};
    return l;
}

node_t *new_node(tile_t *t)
{
    node_t *node = (node_t *)malloc(sizeof(node_t));
    node->next = NULL;
    node->tile = t;
    return node;
}

void add_to_list(list_t *list, node_t *n)
{
    node_t *head = list->head;
    if(!head)
    {
        head = n;
        list->head = head;
        return;
    }

    for(; head->next; head = head->next);
    head->next = n;
}

void free_list(list_t *list)
{
    node_t *head = list->head;

    if(!head)
        return;

    while(head)
    {
        node_t *next = head->next;
        free(head);
        head = next;
    }
}

char list_contains_tile(list_t *list, tile_t *t)
{
    node_t *head = list->head;

    for(; head; head = head->next)
    {
        if(t == head->tile)
            return 1;
    }

    return 0;
}

char list_intersection(list_t *a, list_t *b)
{
    node_t *head = b->head;
    char ret = 0;
    for(; head; head = head->next)
    {
        if(list_contains_tile(a, head->tile))
        {
            ret++;
            break;
        }
    }

    return ret;
}

char list_is_subset(list_t *a, list_t *b)
{
    node_t *head = b->head;
    for(; head; head = head->next)
    {
        if(!list_contains_tile(a, head->tile))
            return 0;
    }
    return 1;
}

tile_t **allocate_array(int m, int n)
{
    tile_t *values = (tile_t *)calloc(m * n, sizeof(tile_t));
    tile_t **rows = (tile_t **)malloc(m * sizeof(tile_t *));
    int i;
    for (i = 0; i < m; ++i)
    {
        rows[i] = values + i * n;
    }

    int x, y;
    for(x = 0; x < m; x++)
    {
        for(y = 0; y < n; y++)
        {
            tile_t *t = &rows[x][y];
            t->x = x;
            t->y = y;
            
            int i;
            for(i = 0; i < 8; i++)
            {
                int nX = x + offsets[i][0];
                int nY = y + offsets[i][1];
                
                if(nX < 0 || nX >= m)
                    continue;
                if(nY < 0 || nY >= n)
                    continue;
                
                t->neighbors[i] = &rows[nX][nY];
            }
        }
    }
    return rows;
}

void set_constraints(int w, int h, int mines)
{
    board_width = w;
    board_height = h;
    board_mines = mines;
    srand(time(0));
}

void assign_vals(tile_t **board)
{
    int x, y, i;
    for(x = 0; x < board_width; x++)
    {
        for(y = 0; y < board_height; y++)
        {
            tile_t *tile = &board[x][y];
            int target = 0;
            for(i = 0; i < 8; i++)
            {
                tile_t *nb = tile->neighbors[i];
                
                if(!nb)
                    continue;

                if(nb->mine)
                    target++;
            }

            tile->nm_count = target;
        }
    }
}

void clear_tile(int x, int y, tile_t **board)
{
    tile_t *tile = &board[x][y];
    tile->cleared = 1;

    if(tile->flagged)
        exit(0);

    // printf("%d\n", tile->nm_count);
    if(!tile->nm_count)
    {
        int i;
        for(i = 0; i < 8; i++)
        {
            tile_t *nb = tile->neighbors[i];
            if(!nb)
                continue;

            if(!nb->cleared)
                clear_tile(nb->x, nb->y, board);
        }
    }
}

tile_t **gen_random_board(int start_x, int start_y)
{
    tile_t **empty = allocate_array(board_width, board_height);

    int mines = 0;

    while(mines < board_mines)
    {
        int nX = rand() % board_width;
        int nY = rand() % board_height;

        if(nX > start_x - 2 && nX < start_x + 2 && nY > start_y - 2 && nY < start_y + 2)
            continue;

        tile_t *tile = &empty[nX][nY];

        if(tile->mine)
            continue;

        tile->mine = 1;
        mines++;
    }
    // printf("Generated random board.\n");
    assign_vals(empty);
    return empty;
}

int adjust_random_board(int start_x, int start_y, tile_t **prev)
{
    tile_t **empty = allocate_array(board_width, board_height);

    int new_needed = 0;
    int mines = 0;

    int i, j;
    for(i = 0; i < board_width; i++)
    {
        for(j = 0; j < board_height; j++)
        {
            tile_t *t = &prev[i][j];

            if(!t->flagged && t->mine)
            {
                new_needed++;
                t->mine = 0;
            }

            t->cleared = 0;
            t->flagged = 0;
            t->nm_count = 0;
        }
    }

    while(mines < new_needed)
    {
        int nX = rand() % board_width;
        int nY = rand() % board_height;

        if(nX > start_x - 2 && nX < start_x + 2 && nY > start_y - 2 && nY < start_y + 2)
            continue;

        tile_t *tile = &prev[nX][nY];

        if(tile->mine)
            continue;

        tile->mine = 1;
        mines++;
    }

    assign_vals(prev);
    return new_needed;
}

tile_t **gen_no_guess_board(int start_x, int start_y)
{
    tile_t **board = gen_random_board(start_x, start_y);
    clear_tile(start_x, start_y, board);
    solve_board(board);

    int its = 0;

    int ppn = -1;
    int pn = 0;

    while(!solve_board(board))
    {
        //free_board(board);
        //board = gen_random_board(start_x, start_y);
        int cn = adjust_random_board(start_x, start_y, board);
        if(cn == pn && cn == ppn)
        {
            // We might be stuck... Best just to nuke it and start over
            free_board(board);
            board = gen_random_board(start_x, start_y);
            pn = 0;
            ppn = -1;
        }
        else
        {
            ppn = pn;
            pn = cn;
        }
        clear_tile(start_x, start_y, board);
        if(its++ > GEN_MAX)
            exit(0);
    }

    // Cleaning
//    int x, y;
//    for(x = 0; x < board_width; x++)
//    {
//        for(y = 0; y < board_height; y++)
//        {
//            tile_t *tile = &board[x][y];
//            tile->cleared = 0;
//            tile->flagged = 0;
//        }
//    }

    printf("Board generated in %d %s.\n", its, its == 1 ? "iteration" : "iterations");

    return board;
}

list_t candidate_list(int x, int y, tile_t **board)
{
    list_t cand = empty_list();
    int candidates = 0;
    tile_t center = board[x][y];
    int i;
    for(i = 0; i < 8; i++)
    {
        tile_t *tile = center.neighbors[i];
        if(!tile)
            continue;
        
        if(!tile->cleared && !tile->flagged)
        {
            add_to_list(&cand, new_node(tile));
            candidates++;
        }
    }
    return cand;
}

char remaining(int x, int y, tile_t **board)
{
    int i;
    tile_t center = board[x][y];
    char rem = center.nm_count;
    for(i = 0; i < 8; i++)
    {
        tile_t *tile = center.neighbors[i];
        if(!tile)
            continue;
        
        if(tile->flagged)
            rem--;
    }

    return rem;
}

char flags(int x, int y, tile_t **board)
{
    tile_t center = board[x][y];
    char rem = 0;
    int i;
    for(i = 0; i < 8; i++)
    {
        tile_t *tile = center.neighbors[i];
        if(!tile)
            continue;
        
        if(tile->flagged)
            rem++;
    }

    return rem;
}

char perform_iteration(tile_t **board)
{
    char move_made = 0;
    int x, y, i;
    for(x = 0; x < board_width; x++)
    {
        for(y = 0; y < board_height; y++)
        {
            tile_t *tile = &board[x][y];
            char rem_a = remaining(x, y, board);

            if(tile->nm_count == 0 || !tile->cleared)
                continue;

            list_t cand_a = candidate_list(x, y, board);

            int flag_a = flags(x, y, board);
            //printf("%d %d\n", flag_a, tile->nm_count);
            if(flag_a == tile->nm_count)
            {
                for(i = 0; i < 8; i++)
                {
                    tile_t *nb = tile->neighbors[i];
                    if(!nb)
                        continue;
                    
                    if(!nb->flagged && !nb->cleared)
                    {
                        clear_tile(nb->x, nb->y, board);
                        move_made = 1;
                    }
                }
            }

            for(i = 0; i < 8; i++)
            {
                tile_t *nb = tile->neighbors[i];
                if(!nb)
                    continue;

                if(!nb->cleared || nb->nm_count == 0)
                    continue;

                list_t cand_b = candidate_list(nb->x, nb->y, board);
                char rem_b = remaining(nb->x, nb->y, board);

                // printf("%d\t %d\n", rem_a, rem_b);

                // First condition
                if(list_is_subset(&cand_b, &cand_a))
                {
                    if(rem_a == rem_b)
                    {
                        node_t *node = cand_b.head;
                        for(; node; node = node->next)
                        {
                            if(!list_contains_tile(&cand_a, node->tile))
                            {
                                clear_tile(node->tile->x, node->tile->y, board);
                                //printf("a\n");
                                move_made = 1;
                            }
                        }
                    }

                    int cbmca = 0;
                    node_t *nd = cand_b.head;
                    for(; nd; nd = nd->next)
                    {
                        if(!list_contains_tile(&cand_a, nd->tile))
                            cbmca++;
                    }
                    if(cbmca == rem_b - rem_a)
                    {
                        node_t *node = cand_b.head;
                        for(; node; node = node->next)
                        {
                            if(!list_contains_tile(&cand_a, node->tile))
                            {
                                node->tile->flagged = 1;
                                if(node->tile->cleared)
                                    exit(0);
                                //printf("b\n");
                                move_made = 1;
                            }
                        }
                    }
                }

                if(rem_a > rem_b)
                {
                    int camcb = 0;
                    node_t *nd = cand_a.head;
                    for(; nd; nd = nd->next)
                    {
                        if(!list_contains_tile(&cand_b, nd->tile))
                            camcb++;
                    }
                    //intersect = list_intersection(&cand_a, &cand_b);
                    if(camcb == rem_a - rem_b)
                    {
                        node_t *node = cand_a.head;
                        for(; node; node = node->next)
                        {
                            if(!list_contains_tile(&cand_b, node->tile))
                            {
                                node->tile->flagged = 1;
                                if(node->tile->cleared)
                                    exit(0);
                                //printf("c\n");
                                move_made = 1;
                            }
                        }
                    }
                }

                free_list(&cand_b);
            }
           
            free_list(&cand_a);
        }
    }
    // printf("%d\n", move_made);
    return move_made;
}

int iterations = 0;
char solve_board(tile_t **board)
{
    while(perform_iteration(board))
    {
        iterations++;
//        if(iterations > GEN_MAX)
//            break;
    }

    int cleared_count = 0;

    int x, y;
    for(x = 0; x < board_width; x++)
    {
        for(y = 0; y < board_height; y++)
        {
            tile_t tile = board[x][y];
            if(tile.cleared)
                cleared_count++;
        }
    }

    return cleared_count == board_width * board_height - board_mines;
}

void free_board(tile_t **board)
{
    free(*board);
    free(board);
}


// int x, y, i;
// for(x = 0; x < board_width; x++)
// {
//     for(y = 0; y < board_height; y++)
//     {
//         for(i = 0; i < 8; i++)
//         {
//             int nX = x + offsets[i][0];
//             int nY = y + offsets[i][1];

//             if(nX < 0 || nX >= board_width)
//                 continue;
//             if(nY < 0 || nY >= board_height)
//                 continue;

//             tile_t tile = board[nX][nY];
//         }
//     }
// }
