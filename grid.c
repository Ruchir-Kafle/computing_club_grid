#include "grid.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

// Total screen dimenions, including extra padding
#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 900

// Number of rows and column, pretty self explanatory
#define ROWS 20
#define COLUMNS 20

// The grid (not the screen) will never exceed these dimensions
#define GRID_MAX_WIDTH 720.0f
#define GRID_MAX_HEIGHT 720.0f

// Border and gap
#define GRID_PADDING 10.0f
#define TILE_GAP 2.0f

// -----------------------------------------------------------------------------
// Types
// -----------------------------------------------------------------------------

// Grid layout configuration information
// Populated in CalculateGridLayout
typedef struct {
    float x;
    float y;
    float width;
    float height;
    float tile_size;
} grid_layout_t;

// -----------------------------------------------------------------------------
// Colors
// ----------------------------------------------------------------------------
// Given RGB color, shift it to Hex.
// Pretty cool math, thanks Chat.
#define HEX_COLOR(hex) (Color){ \
    ((hex >> 16) & 0xFF),       \
    ((hex >> 8) & 0xFF),        \
    (hex & 0xFF),               \
    255                         \
}

static const Color BACKGROUND = HEX_COLOR(0x12141F);
static const Color PANEL       = HEX_COLOR(0x1D2130);
static const Color EMPTY_TILE  = HEX_COLOR(0x2B3041);

// -----------------------------------------------------------------------------
// Grid layout
// -----------------------------------------------------------------------------

/**
 * Calculates the display layout for a given grid so the tiles fit inside the panel.
 *
 * @param grid The grid whose tile count and dimensions should be used to size the layout.
 * @return A grid layout containing the origin, size, and tile dimensions.
 */
static grid_layout_t CalculateGridLayout(const grid_t *grid)
{
    const grid_config_t *config = grid->config;
    const float max_width = config->max_width;
    const float max_height = config->max_height;
    const float padding = config->padding;
    const float tile_gap = config->tile_gap;

    const float available_width = max_width - padding * 2.0f;
    const float available_height = max_height - padding * 2.0f;

    const float tile_width =
        (available_width - tile_gap * (float)(grid->columns - 1)) / (float)grid->columns;

    const float tile_height =
        (available_height - tile_gap * (float)(grid->rows - 1)) / (float)grid->rows;

    const float tile_size = fminf(tile_width, tile_height);

    const float grid_width =
        (float)grid->columns * tile_size + ((float)grid->columns - 1.0f) * tile_gap;

    const float grid_height =
        (float)grid->rows * tile_size + ((float)grid->rows - 1.0f) * tile_gap;

    const float panel_width = grid_width + padding * 2.0f;
    const float panel_height = grid_height + padding * 2.0f;

    grid_layout_t layout = {
        .x = (SCREEN_WIDTH - panel_width) / 2.0f + padding,
        .y = (SCREEN_HEIGHT - panel_height) / 2.0f + padding,
        .width = grid_width,
        .height = grid_height,
        .tile_size = tile_size
    };

    return layout;
}

// -----------------------------------------------------------------------------
// Helper functions (Thanks Chat)
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Creation and lifetime
// -----------------------------------------------------------------------------

/**
 * Creates a new grid with the requested dimensions.
 *
 * @param rows The number of rows in the grid.
 * @param columns The number of columns in the grid.
 * @return A pointer to the allocated grid, or NULL if allocation fails.
 */
grid_t *CreateGrid(size_t rows, size_t columns)
{
    if (rows == 0 || columns == 0) {
        return NULL;
    }

    grid_t *grid = calloc(1, sizeof(*grid));
    if (grid == NULL) {
        return NULL;
    }

    grid->config = calloc(1, sizeof(*grid->config));
    if (grid->config == NULL) {
        free(grid);
        return NULL;
    }

    grid->config->max_width = GRID_MAX_WIDTH;
    grid->config->max_height = GRID_MAX_HEIGHT;
    grid->config->padding = GRID_PADDING;
    grid->config->tile_gap = TILE_GAP;
    grid->config->background_color = BACKGROUND;
    grid->config->panel_color = PANEL;
    grid->config->default_tile_color = EMPTY_TILE;
    grid->config->rounded_panel = true;

    grid->rows = rows;
    grid->columns = columns;
    grid->tiles = calloc(rows * columns, sizeof(*grid->tiles));
    if (grid->tiles == NULL) {
        free(grid->config);
        free(grid);
        return NULL;
    }

    for (size_t i = 0; i < rows * columns; i++) {
        grid->tiles[i].contents = NULL;
        grid->tiles[i].state = NULL;
        grid->tiles[i].color = grid->config->default_tile_color;
    }

    return grid;
}

/**
 * Frees the memory used by a grid.
 *
 * @param grid The grid to destroy.
 */
void DestroyGrid(grid_t *grid)
{
    if (grid == NULL) {
        return;
    }

    free(grid->tiles);
    free(grid->config);
    free(grid);
}

// -----------------------------------------------------------------------------
// Position and lookup
// -----------------------------------------------------------------------------

/**
 * Returns whether a given row and column are within the bounds of a grid.
 *
 * @param grid The grid to validate against.
 * @param row The row index to check.
 * @param column The column index to check.
 * @return true if the position is valid, false otherwise.
 */
bool GridIsValidPosition(const grid_t *grid, int row, int column)
{
    if (grid == NULL) {
        return false;
    }

    return row >= 0 && row < (int)grid->rows &&
           column >= 0 && column < (int)grid->columns;
}

grid_tile_t *GetTile(grid_t *grid, int row, int column);

/**
 * Finds the neighbor of a tile in a given direction.
 *
 * @param grid The grid to inspect.
 * @param row The current tile row.
 * @param column The current tile column.
 * @param direction The direction to move in.
 * @return A pointer to the adjacent tile, or NULL if no neighbor exists.
 */
grid_tile_t *GridGetNeighbor(grid_t *grid, int row, int column, grid_direction_t direction)
{
    int next_row = row;
    int next_column = column;

    switch (direction) {
        case DIR_UP:
            next_row -= 1;
            break;
        case DIR_RIGHT:
            next_column += 1;
            break;
        case DIR_DOWN:
            next_row += 1;
            break;
        case DIR_LEFT:
            next_column -= 1;
            break;
        case DIR_UP_RIGHT:
            next_row -= 1;
            next_column += 1;
            break;
        case DIR_DOWN_RIGHT:
            next_row += 1;
            next_column += 1;
            break;
        case DIR_DOWN_LEFT:
            next_row += 1;
            next_column -= 1;
            break;
        case DIR_UP_LEFT:
            next_row -= 1;
            next_column -= 1;
            break;
        default:
            return NULL;
    }

    return GetTile(grid, next_row, next_column);
}

/**
 * Converts a linear tile index into a row and column pair.
 *
 * @param grid The grid containing the tile.
 * @param index The linear tile index.
 * @return A row/column position for the index, or {-1, -1} if invalid.
 */
grid_position_t GetPositionFromIndex(const grid_t *grid, int index)
{
    grid_position_t position = { .row = -1, .column = -1 };

    if (grid == NULL || index < 0 || index >= (int)(grid->rows * grid->columns)) {
        return position;
    }

    position.row = index / (int)grid->columns;
    position.column = index % (int)grid->columns;
    return position;
}

/**
 * Converts a row and column into the corresponding linear tile index.
 *
 * @param grid The grid containing the target tile.
 * @param row The row index.
 * @param column The column index.
 * @return The linear index for that position, or -1 if the tile is out of bounds.
 */
int GetIndexFromPosition(const grid_t *grid, int row, int column)
{
    if (!GridIsValidPosition(grid, row, column)) {
        return -1;
    }

    return row * (int)grid->columns + column;
}

/**
 * Retrieves a tile by its linear index.
 *
 * @param grid The grid to access.
 * @param index The linear tile index.
 * @return A pointer to the requested tile, or NULL if the index is invalid.
 */
grid_tile_t *GetTileAtIndex(grid_t *grid, int index)
{
    if (grid == NULL || index < 0 || index >= (int)(grid->rows * grid->columns)) {
        return NULL;
    }

    return &grid->tiles[index];
}

/**
 * Retrieves a tile by row and column.
 *
 * @param grid The grid to access.
 * @param row The tile row.
 * @param column The tile column.
 * @return A pointer to the requested tile, or NULL if the tile is out of bounds.
 */
grid_tile_t *GetTile(grid_t *grid, int row, int column)
{
    if (!GridIsValidPosition(grid, row, column)) {
        return NULL;
    }

    return &grid->tiles[row * (int)grid->columns + column];
}

/**
 * Set the contents of a tile.
 *
 * @param grid The grid to update.
 * @param row The tile row.
 * @param column The tile column.
 * @param contents The contents to assign to the tile.
 */
void SetTileContents(grid_t *grid, int row, int column, void *contents)
{
    grid_tile_t *tile = GetTile(grid, row, column);

    if (tile == NULL) return;

    tile->contents = contents;
}

/**
 * Set the state of a tile.
 *
 * @param grid The grid to update.
 * @param row The tile row.
 * @param column The tile column.
 * @param state The state to assign to the tile.
 */
void SetTileState(grid_t *grid, int row, int column, void *state)
{
    grid_tile_t *tile = GetTile(grid, row, column);

    if (tile == NULL) return;

    tile->state = state;
}

/**
 * Change the color of a tile.
 *
 * @param grid The grid to update.
 * @param row The tile row.
 * @param column The tile column.
 * @param color The new color for the tile.
 */
void SetTileColor(grid_t *grid, int row, int column, Color color)
{
    grid_tile_t *tile = GetTile(grid, row, column);

    if (tile == NULL) return;

    tile->color = color;
}

// -----------------------------------------------------------------------------
// Tile mutation
// -----------------------------------------------------------------------------

/**
 * Reset a tile to its empty default state.
 *
 * @param grid The grid to update.
 * @param row The tile row.
 * @param column The tile column.
 */
void ResetTile(grid_t *grid, int row, int column)
{
    grid_tile_t *tile = GetTile(grid, row, column);

    if (tile == NULL) return;

    tile->contents = NULL;
    tile->state = NULL;
    tile->color = grid->config->default_tile_color;
}

/**
 * Clears every tile on the grid back to its empty default state.
 *
 * @param grid The grid to clear.
 */
void GridClear(grid_t *grid)
{
    if (grid == NULL) {
        return;
    }

    for (size_t i = 0; i < grid->rows * grid->columns; i++) {
        grid->tiles[i].contents = NULL;
        grid->tiles[i].state = NULL;
        grid->tiles[i].color = grid->config->default_tile_color;
    }
}

/**
 * Fills every tile in the grid with the specified color.
 *
 * @param grid The grid to update.
 * @param color The color to assign to every tile.
 */
void GridFill(grid_t *grid, Color color)
{
    if (grid == NULL) {
        return;
    }

    for (size_t i = 0; i < grid->rows * grid->columns; i++) {
        grid->tiles[i].color = color;
    }
}

/**
 * Sets every tile's contents pointer to the same value.
 *
 * @param grid The grid to update.
 * @param contents The contents pointer to assign.
 */
void GridSetAllContents(grid_t *grid, void *contents)
{
    if (grid == NULL) {
        return;
    }

    for (size_t i = 0; i < grid->rows * grid->columns; i++) {
        grid->tiles[i].contents = contents;
    }
}

/**
 * Updates the color of a single tile.
 *
 * @param grid The grid to update.
 * @param row The tile row.
 * @param column The tile column.
 * @param color The color to assign.
 */
void GridSetColor(grid_t *grid, int row, int column, Color color)
{
    SetTileColor(grid, row, column, color);
}

// -----------------------------------------------------------------------------
// Rendering
// -----------------------------------------------------------------------------

/**
 * Renders the grid using the current configuration values.
 *
 * @param grid The grid to draw.
 */
void GridDraw(grid_t *grid)
{
    if (grid == NULL) {
        return;
    }

    const grid_layout_t layout = CalculateGridLayout(grid);
    const grid_config_t *config = grid->config;

    Rectangle panel = {
        layout.x - config->padding,
        layout.y - config->padding,
        layout.width + config->padding * 2.0f,
        layout.height + config->padding * 2.0f
    };

    if (config->rounded_panel) {
        DrawRectangleRounded(panel, 0.035f, 12, config->panel_color);
    } else {
        DrawRectangleRec(panel, config->panel_color);
    }

    for (size_t row = 0; row < grid->rows; row++) {
        for (size_t column = 0; column < grid->columns; column++) {
            const int index = (int)(row * grid->columns + column);
            const grid_tile_t *tile = &grid->tiles[index];

            const float x =
                layout.x + (float)column * (layout.tile_size + config->tile_gap);

            const float y =
                layout.y + (float)row * (layout.tile_size + config->tile_gap);

            Rectangle tile_rect = {
                x,
                y,
                layout.tile_size,
                layout.tile_size
            };

            DrawRectangleRec(tile_rect, tile->color);
        }
    }
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------

int main(void)
{
    grid_t *grid = CreateGrid(ROWS, COLUMNS);

    if (grid == NULL) {
        fprintf(stderr, "Could not allocate grid memory\n");
        return EXIT_FAILURE;
    }

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tetris Grid");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(BACKGROUND);

        GridDraw(grid);

        EndDrawing();
    }

    DestroyGrid(grid);
    CloseWindow();

    return EXIT_SUCCESS;
}
