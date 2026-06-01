#ifndef PHYSICS_H
#define PHYSICS_H

#include "game_ctx.h"

typedef enum CollisionCellType {
    COLLISION_CELL_FREE = 0,
    COLLISION_CELL_SOLID = 1,
    COLLISION_CELL_TRIGGER = 2
} CollisionCellType;

typedef struct CollisionMatrix {
    int width;
    int height;
    Vector2 origin;
    uint8_t* cells;
} CollisionMatrix;

typedef void (*PhysicsTriggerFn)(void* actor, uint8_t cell_type, Vector2 cell_world_pos, GameContext* ctx);

typedef struct PhysicsMoveResult {
    Vector2 position;
    bool blocked_x;
    bool blocked_y;
    bool triggered;
    Vector2 trigger_cell;
    uint8_t trigger_type;
} PhysicsMoveResult;

static Vector2 physics_half_size(Vector2 size)
{
    return new_Vector2(size.x / 2, size.y / 2);
}

static Vector2 physics_body_min(Vector2 position, Vector2 size)
{
    return vector_subtr(position, physics_half_size(size));
}

static Vector2 physics_body_max(Vector2 position, Vector2 size)
{
    Vector2 min_pos = physics_body_min(position, size);
    return new_Vector2(min_pos.x + size.x - 1, min_pos.y + size.y - 1);
}

static void collision_matrix_init(CollisionMatrix* matrix, int width, int height, Vector2 origin)
{
    if (matrix == NULL)
        return;

    matrix->width = width;
    matrix->height = height;
    matrix->origin = origin;
    matrix->cells = (uint8_t*)calloc((size_t)(width * height), sizeof(uint8_t));
}

static void collision_matrix_destroy(CollisionMatrix* matrix)
{
    if (matrix == NULL)
        return;

    free(matrix->cells);
    matrix->cells = NULL;
    matrix->width = 0;
    matrix->height = 0;
    matrix->origin = VETOR_NULO;
}

static bool collision_matrix_in_bounds(const CollisionMatrix* matrix, int x, int y)
{
    if (matrix == NULL || matrix->cells == NULL)
        return false;

    return x >= 0 && x < matrix->width && y >= 0 && y < matrix->height;
}

static uint8_t collision_matrix_get(const CollisionMatrix* matrix, int x, int y)
{
    if (!collision_matrix_in_bounds(matrix, x, y))
        return COLLISION_CELL_FREE;

    return matrix->cells[y * matrix->width + x];
}

static void collision_matrix_set(CollisionMatrix* matrix, int x, int y, uint8_t value)
{
    if (!collision_matrix_in_bounds(matrix, x, y))
        return;

    matrix->cells[y * matrix->width + x] = value;
}

static void collision_matrix_fill(CollisionMatrix* matrix, uint8_t value)
{
    if (matrix == NULL || matrix->cells == NULL)
        return;

    for (int y = 0; y < matrix->height; y++)
        for (int x = 0; x < matrix->width; x++)
            collision_matrix_set(matrix, x, y, value);
}

static Vector2 collision_matrix_cell_world(const CollisionMatrix* matrix, int x, int y)
{
    return vector_sum(matrix->origin, new_Vector2(x, y));
}

static bool physics_body_overlaps_solid_cell(const CollisionMatrix* matrix, Vector2 position, Vector2 size, int* out_cell_x, int* out_cell_y)
{
    if (matrix == NULL || matrix->cells == NULL)
        return false;

    Vector2 min_pos = physics_body_min(position, size);
    Vector2 max_pos = physics_body_max(position, size);

    for (int y = 0; y < matrix->height; y++)
    {
        for (int x = 0; x < matrix->width; x++)
        {
            if (collision_matrix_get(matrix, x, y) != COLLISION_CELL_SOLID)
                continue;

            Vector2 cell = collision_matrix_cell_world(matrix, x, y);
            if (cell.x < min_pos.x || cell.x > max_pos.x || cell.y < min_pos.y || cell.y > max_pos.y)
                continue;

            if (out_cell_x != NULL)
                *out_cell_x = x;
            if (out_cell_y != NULL)
                *out_cell_y = y;
            return true;
        }
    }

    return false;
}

static bool physics_body_has_support(const CollisionMatrix* matrix, Vector2 position, Vector2 size)
{
    if (matrix == NULL || matrix->cells == NULL)
        return false;

    Vector2 min_pos = physics_body_min(position, size);
    Vector2 max_pos = physics_body_max(position, size);
    int support_y_world = max_pos.y + 1;

    for (int x = min_pos.x; x <= max_pos.x; x++)
    {
        int matrix_x = x - matrix->origin.x;
        int matrix_y = support_y_world - matrix->origin.y;
        if (collision_matrix_get(matrix, matrix_x, matrix_y) == COLLISION_CELL_SOLID)
            return true;
    }

    return false;
}

static void physics_scan_triggers(const CollisionMatrix* matrix, Vector2 position, Vector2 size, void* actor, GameContext* ctx, PhysicsTriggerFn on_trigger)
{
    if (matrix == NULL || matrix->cells == NULL || on_trigger == NULL)
        return;

    Vector2 min_pos = physics_body_min(position, size);
    Vector2 max_pos = physics_body_max(position, size);

    for (int y = 0; y < matrix->height; y++)
    {
        for (int x = 0; x < matrix->width; x++)
        {
            if (collision_matrix_get(matrix, x, y) != COLLISION_CELL_TRIGGER)
                continue;

            Vector2 cell = collision_matrix_cell_world(matrix, x, y);
            if (cell.x < min_pos.x || cell.x > max_pos.x || cell.y < min_pos.y || cell.y > max_pos.y)
                continue;

            on_trigger(actor, COLLISION_CELL_TRIGGER, cell, ctx);
        }
    }
}

static Vector2 physics_resolve_axis_motion(
    const CollisionMatrix* matrix,
    Vector2 position,
    Vector2 size,
    int delta_x,
    int delta_y,
    void* actor,
    GameContext* ctx,
    PhysicsTriggerFn on_trigger,
    bool* blocked_x,
    bool* blocked_y
)
{
    if (blocked_x != NULL)
        *blocked_x = false;
    if (blocked_y != NULL)
        *blocked_y = false;

    int remaining_x = delta_x;
    while (remaining_x != 0)
    {
        int step = (remaining_x > 0) ? 1 : -1;
        Vector2 candidate = vector_sum(position, new_Vector2(step, 0));

        if (physics_body_overlaps_solid_cell(matrix, candidate, size, NULL, NULL))
        {
            if (blocked_x != NULL)
                *blocked_x = true;
            break;
        }

        physics_scan_triggers(matrix, candidate, size, actor, ctx, on_trigger);
        position = candidate;
        remaining_x -= step;
    }

    int remaining_y = delta_y;
    while (remaining_y != 0)
    {
        int step = (remaining_y > 0) ? 1 : -1;
        Vector2 candidate = vector_sum(position, new_Vector2(0, step));

        if (physics_body_overlaps_solid_cell(matrix, candidate, size, NULL, NULL))
        {
            if (blocked_y != NULL)
                *blocked_y = true;
            break;
        }

        physics_scan_triggers(matrix, candidate, size, actor, ctx, on_trigger);
        position = candidate;
        remaining_y -= step;
    }

    return position;
}

static PhysicsMoveResult physics_resolve_motion(
    const CollisionMatrix* matrix,
    Vector2 position,
    Vector2 size,
    Vector2 delta,
    void* actor,
    GameContext* ctx,
    PhysicsTriggerFn on_trigger
)
{
    PhysicsMoveResult result = {0};
    result.position = physics_resolve_axis_motion(
        matrix,
        position,
        size,
        delta.x,
        delta.y,
        actor,
        ctx,
        on_trigger,
        &result.blocked_x,
        &result.blocked_y
    );
    return result;
}

static void physics_draw_collision_matrix(
    CollisionMatrix* matrix,
    Screen* screen,
    Vector2 actor_pos,
    Vector2 actor_size,
    bool skip_actor_body
)
{
    if (matrix == NULL || matrix->cells == NULL || screen == NULL)
        return;

    Vector2 actor_min = physics_body_min(actor_pos, actor_size);
    Vector2 actor_max = physics_body_max(actor_pos, actor_size);
    int actor_max_x_with_padding = actor_max.x + 1;

    for (int y = 0; y < matrix->height; y++)
    {
        for (int x = 0; x < matrix->width; x++)
        {
            Vector2 world_cell = collision_matrix_cell_world(matrix, x, y);

            if (skip_actor_body)
            {
                bool inside_actor = (
                    world_cell.x >= actor_min.x && world_cell.x <= actor_max_x_with_padding &&
                    world_cell.y >= actor_min.y && world_cell.y <= actor_max.y
                );
                if (inside_actor)
                    continue;
            }

            Vector2 screen_pos;
            if (!vetor_aponta_para_area_visivel(screen, world_cell, &screen_pos))
                continue;

            uint8_t cell = collision_matrix_get(matrix, x, y);
            moveCursor(screen_pos);
            printf("%u", cell);
        }
    }
}

#endif