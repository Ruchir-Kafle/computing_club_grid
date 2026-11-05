#ifndef GRID_H
#define GRID_H

#include <stdbool.h>
#include <stddef.h>
#include "raylib.h"

typedef struct grid_config_t {
    float max_width;
    float max_height;
    float padding;
    float tile_gap;

    Color background_color;
    Color panel_color;
    Color default_tile_color;

    bool rounded_panel;
} grid_config_t;

typedef struct {
    void *contents;
    void *state;
    Color color;
} grid_tile_t;

typedef struct {
    int row;
    int column;
} grid_position_t;

typedef struct {
    size_t rows;
    size_t columns;
    grid_tile_t *tiles;
    grid_config_t *config;
} grid_t;

typedef enum {
    DIR_UP,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT,
    DIR_UP_RIGHT,
    DIR_DOWN_RIGHT,
    DIR_DOWN_LEFT,
    DIR_UP_LEFT
} grid_direction_t;

grid_t *CreateGrid(size_t rows, size_t columns);
void DestroyGrid(grid_t *grid);

bool GridIsValidPosition(const grid_t *grid, int row, int column);
grid_position_t GetPositionFromIndex(const grid_t *grid, int index);
int GetIndexFromPosition(const grid_t *grid, int row, int column);
grid_tile_t *GetTileAtIndex(grid_t *grid, int index);
grid_tile_t *GetTile(grid_t *grid, int row, int column);
grid_tile_t *GridGetNeighbor(grid_t *grid, int row, int column, grid_direction_t direction);

void SetTileContents(grid_t *grid, int row, int column, void *contents);
void SetTileState(grid_t *grid, int row, int column, void *state);
void SetTileColor(grid_t *grid, int row, int column, Color color);
void ResetTile(grid_t *grid, int row, int column);

void GridClear(grid_t *grid);
void GridFill(grid_t *grid, Color color);
void GridSetAllContents(grid_t *grid, void *contents);
void GridSetColor(grid_t *grid, int row, int column, Color color);
void GridDraw(grid_t *grid);

#endif
