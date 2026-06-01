#ifndef LEVEL_H
#define LEVEL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "physics.h"
#include "code_editor_runtime.h"
#include "player.h"
#include "entity.h"

typedef enum LevelExitReason {
    LEVEL_EXIT_NONE = 0,
    LEVEL_EXIT_COMPLETED,
    LEVEL_EXIT_RESTART,
    LEVEL_EXIT_ABORTED,
    LEVEL_EXIT_QUIT
} LevelExitReason;

typedef struct LevelContext {
    GameContext* game;
    int level_index;
    int total_levels;
    uint64_t frame;
    char last_input;
    bool debug_enabled;
    void* campaign_state;
} LevelContext;

typedef struct LevelState {
    CollisionMatrix collision_map;
    Player* player;
    Vector2 player_spawn;
    Entity** entities;
    int entity_count;
    int entity_capacity;
    CodeBindingRegistry binding_registry;
    CodeEditorSession editor_session;
    CodeCursor cursor;
    CodeEditorChrome chrome;
    ViewMode last_view_mode;
} LevelState;

typedef struct LevelInstance LevelInstance;

typedef bool (*LevelInitFn)(LevelInstance* level, LevelContext* ctx);
typedef void (*LevelInputFn)(LevelInstance* level, LevelContext* ctx, char input);
typedef void (*LevelTickFn)(LevelInstance* level, LevelContext* ctx);
typedef void (*LevelDrawFn)(LevelInstance* level, LevelContext* ctx);
typedef void (*LevelDestroyFn)(LevelInstance* level, LevelContext* ctx);

typedef struct LevelVTable {
    LevelInitFn init;
    LevelInputFn handle_input;
    LevelTickFn tick;
    LevelDrawFn draw;
    LevelDestroyFn destroy;
} LevelVTable;

typedef struct LevelDefinition {
    const char* id;
    const char* display_name;
    Vector2 world_size;
    bool allow_camera_move;
    LevelVTable vtable;
} LevelDefinition;

struct LevelInstance {
    const LevelDefinition* definition;
    void* state;
    LevelState common;
    bool initialized;
    bool running;
    LevelExitReason exit_reason;
    int next_level_index;
};

static inline void level_state_init(LevelState* state, ViewMode initial_view_mode)
{
    if (state == NULL)
        return;

    memset(state, 0, sizeof(LevelState));
    state->last_view_mode = initial_view_mode;
    state->cursor.position = VETOR_NULO;
    state->player_spawn = VETOR_NULO;
}

static inline bool level_state_add_entity(LevelState* state, Entity* entity, int* out_index)
{
    if (state == NULL || entity == NULL)
        return false;

    if (state->entity_count == state->entity_capacity)
    {
        int new_capacity = (state->entity_capacity == 0) ? 4 : state->entity_capacity * 2;
        Entity** resized = (Entity**)realloc(state->entities, new_capacity * sizeof(Entity*));
        if (resized == NULL)
            return false;

        state->entities = resized;
        state->entity_capacity = new_capacity;
    }

    int index = state->entity_count;
    state->entities[state->entity_count++] = entity;

    if (out_index != NULL)
        *out_index = index;

    return true;
}

static inline Entity* level_state_entity_at(LevelState* state, int index)
{
    if (state == NULL || index < 0 || index >= state->entity_count)
        return NULL;

    return state->entities[index];
}

static inline void level_state_draw_entities(LevelState* state, GameContext* ctx)
{
    if (state == NULL || ctx == NULL)
        return;

    for (int i = 0; i < state->entity_count; i++)
        entity_draw_current_view(state->entities[i], ctx);
}

static inline void level_state_hide_entities(LevelState* state, GameContext* ctx)
{
    if (state == NULL || ctx == NULL)
        return;

    for (int i = 0; i < state->entity_count; i++)
        entity_hide_current_view(state->entities[i], ctx);
}

static inline void game_clear_all_screens(GameContext* ctx)
{
    if (ctx == NULL)
        return;

    for (int i = 0; i < QTD_DEBUG_TYPES; i++)
        limpar_buffer(ctx->screens[i]);
}

static inline bool level_state_body_overlaps_cell_type(
    const LevelState* state,
    Vector2 position,
    Vector2 size,
    uint8_t cell_type
)
{
    if (state == NULL || state->collision_map.cells == NULL)
        return false;

    Vector2 min_pos = physics_body_min(position, size);
    Vector2 max_pos = physics_body_max(position, size);

    for (int y = 0; y < state->collision_map.height; y++)
    {
        for (int x = 0; x < state->collision_map.width; x++)
        {
            if (collision_matrix_get(&state->collision_map, x, y) != cell_type)
                continue;

            Vector2 cell_world = collision_matrix_cell_world(&state->collision_map, x, y);
            if (cell_world.x < min_pos.x || cell_world.x > max_pos.x)
                continue;
            if (cell_world.y < min_pos.y || cell_world.y > max_pos.y)
                continue;

            return true;
        }
    }

    return false;
}

static inline bool level_state_player_overlaps_cell_type(const LevelState* state, uint8_t cell_type)
{
    if (state == NULL || state->player == NULL || state->player->sprite == NULL)
        return false;

    return level_state_body_overlaps_cell_type(
        state,
        state->player->position,
        state->player->sprite->size,
        cell_type
    );
}

static inline void level_state_resolve_player_entity_collisions(LevelState* state)
{
    if (state == NULL || state->player == NULL || state->player->sprite == NULL)
        return;

    Player* player = state->player;
    Vector2 player_size = player->sprite->size;
    Vector2 player_min = physics_body_min(player->position, player_size);
    Vector2 player_max = physics_body_max(player->position, player_size);

    for (int i = 0; i < state->entity_count; i++)
    {
        Entity* entity = state->entities[i];
        if (entity == NULL || !entity->collidable)
            continue;

        Vector2 entity_min = physics_body_min(entity->position, entity->size);
        Vector2 entity_max = physics_body_max(entity->position, entity->size);

        bool overlap_x = !(player_max.x < entity_min.x || player_min.x > entity_max.x);
        bool overlap_y = !(player_max.y < entity_min.y || player_min.y > entity_max.y);
        if (!overlap_x || !overlap_y)
            continue;

        int overlap_left = player_max.x - entity_min.x + 1;
        int overlap_right = entity_max.x - player_min.x + 1;
        int overlap_up = player_max.y - entity_min.y + 1;
        int overlap_down = entity_max.y - player_min.y + 1;

        int push_x = overlap_left < overlap_right ? -overlap_left : overlap_right;
        int push_y = overlap_up < overlap_down ? -overlap_up : overlap_down;

        if (abs(push_x) <= abs(push_y))
        {
            player->position.x += push_x;
            player->velocity.x = 0;
        }
        else
        {
            player->position.y += push_y;
            player->velocity.y = 0;
            if (push_y < 0)
                player->grounded = true;
        }

        player->sprite->position = player->position;

        player_min = physics_body_min(player->position, player_size);
        player_max = physics_body_max(player->position, player_size);
    }
}

static inline void level_state_reset_player(LevelState* state)
{
    if (state == NULL || state->player == NULL)
        return;

    state->player->position = state->player_spawn;
    state->player->velocity = VETOR_NULO;
    state->player->grounded = false;
    state->player->sprite->position = state->player->position;
}

static inline void level_state_destroy(LevelState* state)
{
    if (state == NULL)
        return;

    if (state->player != NULL)
    {
        destroy_player(state->player);
        state->player = NULL;
    }

    for (int i = 0; i < state->entity_count; i++)
        destroy_entity(state->entities[i]);

    free(state->entities);
    state->entities = NULL;
    state->entity_count = 0;
    state->entity_capacity = 0;
    collision_matrix_destroy(&state->collision_map);
    codeeditor_destroy(&state->editor_session);
    codebinding_registry_destroy(&state->binding_registry);
}

static inline LevelContext level_create_context(
    GameContext* game,
    int level_index,
    int total_levels,
    void* campaign_state
)
{
    LevelContext ctx;
    ctx.game = game;
    ctx.level_index = level_index;
    ctx.total_levels = total_levels;
    ctx.frame = 0;
    ctx.last_input = '\0';
    ctx.debug_enabled = true;
    ctx.campaign_state = campaign_state;
    return ctx;
}

static inline void level_instance_reset(
    LevelInstance* level, LevelContext* ctx, const LevelDefinition* definition, void* state
)
{
    if (level == NULL)
        return;

    level->definition = definition;
    level->state = state;
    level_state_init(&level->common, VISUAL);
    level->initialized = false;
    level->running = false;
    level->exit_reason = LEVEL_EXIT_NONE;
    level->next_level_index = -1;
    game_clear_all_screens(ctx->game);
}

static inline void level_request_exit(LevelInstance* level, LevelExitReason reason, int next_level_index)
{
    if (level == NULL)
        return;

    level->running = false;
    level->exit_reason = reason;
    level->next_level_index = next_level_index;
}

static inline bool level_start(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL || level->definition == NULL || ctx == NULL)
        return false;

    bool ok = true;
    if (level->definition->vtable.init != NULL)
        ok = level->definition->vtable.init(level, ctx);

    level->initialized = ok;
    level->running = ok;
    level->exit_reason = LEVEL_EXIT_NONE;
    level->next_level_index = -1;
    return ok;
}

static inline void level_step_input(LevelInstance* level, LevelContext* ctx, char input)
{
    if (level == NULL || ctx == NULL || !level->running || level->definition == NULL)
        return;

    ctx->last_input = input;
    if (level->definition->vtable.handle_input != NULL)
        level->definition->vtable.handle_input(level, ctx, input);
}

static inline void level_step_tick(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL || ctx == NULL || !level->running || level->definition == NULL)
        return;

    if (level->definition->vtable.tick != NULL)
        level->definition->vtable.tick(level, ctx);

    ctx->frame += 1;
}

static inline void level_step_draw(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL || ctx == NULL || !level->running || level->definition == NULL)
        return;

    if (level->definition->vtable.draw != NULL)
        level->definition->vtable.draw(level, ctx);
}

static inline void level_stop(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL || level->definition == NULL)
        return;

    if (level->definition->vtable.destroy != NULL)
        level->definition->vtable.destroy(level, ctx);

    level->running = false;
}

#endif
