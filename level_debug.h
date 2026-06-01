#ifndef LEVEL_DEBUG_H
#define LEVEL_DEBUG_H

#define USE_SHORTCUTS
#include "include/graphycs.h"
#include "physics.h"
#include "code_editor_runtime.h"
#include "player.h"

typedef struct DebugLevelState {
    CollisionMatrix collision_map;
    Player* player;
    Entity* demo_entity;
    Entity* wall;
    CodeBindingRegistry binding_registry;
    CodeEditorSession editor_session;
    CodeCursor cursor;
    CodeEditorChrome chrome;
    ViewMode last_view_mode;
} DebugLevelState;

static void debug_level_player_on_triggering(Player* player, uint8_t cell_type, Vector2 cell_pos, GameContext* ctx)
{
    if (player == NULL || cell_type != COLLISION_CELL_TRIGGER)
        return;

    player_apply_state(player, PLAYER_STATE_HIT);
    (void)cell_pos;
    (void)ctx;
}

static void debug_level_player_trigger_proxy(void* actor, uint8_t cell_type, Vector2 cell_pos, GameContext* ctx)
{
    Player* player = (Player*)actor;
    if (player == NULL || player->on_triggering == NULL)
        return;

    player->on_triggering(player, cell_type, cell_pos, ctx);
}

static void debug_level_setup_collision_map(DebugLevelState* state)
{
    if (state == NULL)
        return;

    collision_matrix_init(
        &state->collision_map,
        SCREEN_SIZE_X,
        SCREEN_SIZE_Y,
        new_Vector2(-(SCREEN_SIZE_X / 2), -(SCREEN_SIZE_Y / 2))
    );
    collision_matrix_fill(&state->collision_map, COLLISION_CELL_FREE);

    for (int x = 0; x < state->collision_map.width; x++)
        collision_matrix_set(&state->collision_map, x, state->collision_map.height - 1, COLLISION_CELL_SOLID);

    for (int x = 72; x <= 92; x++)
        collision_matrix_set(&state->collision_map, x, 27, COLLISION_CELL_SOLID);

    for (int y = 18; y <= 26; y++)
        collision_matrix_set(&state->collision_map, 105, y, COLLISION_CELL_SOLID);

    collision_matrix_set(&state->collision_map, 110, 20, COLLISION_CELL_TRIGGER);
    collision_matrix_set(&state->collision_map, 86, 22, COLLISION_CELL_TRIGGER);
}

CodeFile* create_debug_level_codefile()
{
    CodeFile* file = create_codefile();
    set_codefile_terminal_prompt(file, "codeworld@level1:/map/structure$ cat codefile.c");

    CodeLine* line = push_back_empty_codeline(file);
    push_immutable_token(line, "const", criar_cor(220, 200, 90));
    push_immutable_token(line, "int", criar_cor(90, 220, 110));
    push_immutable_token(line, "bridge_is_extended", criar_cor(90, 150, 230));
    push_immutable_token(line, "=", criar_cor(220, 90, 90));
    push_immutable_token(line, "true", criar_cor(180, 120, 220));
    push_immutable_token(line, ";", criar_cor(220, 200, 90));

    line = push_back_empty_codeline(file);
    push_immutable_token(line, "if", criar_cor(90, 220, 110));
    push_immutable_token(line, "(", criar_cor(220, 200, 90));
    push_immutable_token(line, "bridge_is_extended", criar_cor(90, 150, 230));
    push_immutable_token(line, ")", criar_cor(220, 200, 90));
    push_immutable_token(line, "{", criar_cor(220, 90, 90));

    line = push_back_empty_codeline(file);
    push_indent_token(line, 4);
    push_immutable_token(line, "unlock_bridge", criar_cor(90, 150, 230));
    push_immutable_token(line, "();", criar_cor(220, 200, 90));

    line = push_back_empty_codeline(file);
    push_immutable_token(line, "}", criar_cor(220, 90, 90));

    return file;
}

ObjetoComplexo* create_debug_level_entity_representation()
{
    Objeto** frames = (Objeto**)malloc(QTD_DEBUG_TYPES * sizeof(Objeto*));
    Color palette[QTD_DEBUG_TYPES] = {
        criar_cor(220, 90, 90),
        criar_cor(90, 220, 110),
        criar_cor(90, 150, 230),
        criar_cor(220, 200, 90),
        criar_cor(180, 120, 220)
    };

    for (int i = 0; i < QTD_DEBUG_TYPES; i++)
    {
        frames[i] = criar_retangulo_monocromatico(palette[i], nv2(12, 4));
        frames[i]->position = VETOR_NULO;
    }

    ObjetoComplexo* representation = criar_objeto_complexo_via_lista(frames, QTD_DEBUG_TYPES);
    centralizar_objeto_complexo(representation);
    return representation;
}

void debug_wall_on_token(Entity* wall, CodeToken* token, GameContext* ctx)
{
    if (wall == NULL || token == NULL)
        return;

    if (token->type == TOKEN_NULL)
        return;

    bool should_show = (strcmp(token->string, "true") == 0);
    entity_set_visible(wall, should_show);

    if (should_show)
        entity_draw_current_view(wall, ctx);
    else
        entity_hide_current_view(wall, ctx);
}

bool setup_debug_level_bindings(CodeBindingRegistry* registry, Entity* wall)
{
    if (registry == NULL || wall == NULL)
        return false;

    return codebinding_registry_add(registry, wall, 0, 4, debug_wall_on_token);
}

void debug_level_init(DebugLevelState* state, GameContext* ctx)
{
    if (state == NULL || ctx == NULL)
        return;

    debug_level_setup_collision_map(state);

    state->player = create_player(new_Vector2(-20, 6));
    state->player->user_data = state;
    state->player->on_triggering = debug_level_player_on_triggering;

    state->demo_entity = create_entity(new_Vector2(20, 0), &((Vector2){12, 4}), NULL);
    entity_attach_representation(state->demo_entity, create_debug_level_entity_representation(), true);
    entity_set_visible(state->demo_entity, true);
    state->demo_entity->collidable = true;

    Objeto** wall_frames = (Objeto**)malloc(1 * sizeof(Objeto*));
    wall_frames[0] = criar_retangulo_monocromatico(criar_cor(200, 180, 60), nv2(8, 3));
    ObjetoComplexo* wall_rep = criar_objeto_complexo_via_lista(wall_frames, 1);
    centralizar_objeto_complexo(wall_rep);
    state->wall = create_entity(new_Vector2(8, 0), &((Vector2){8, 3}), NULL);
    entity_attach_representation(state->wall, wall_rep, true);
    entity_set_visible(state->wall, true);
    state->wall->collidable = true;

    codebinding_registry_init(&state->binding_registry);
    codeeditor_init(&state->editor_session, create_debug_level_codefile());
    codeeditor_set_bindings(&state->editor_session, &state->binding_registry);
    setup_debug_level_bindings(&state->binding_registry, state->wall);

    state->cursor = (CodeCursor){
        .position = VETOR_NULO,
        .bg_color = criar_cor(25, 240, 255),
        .imm_bg_color = criar_cor(255, 235, 40)
    };

    state->chrome = (CodeEditorChrome){
        .code_origin = new_Vector2(2, 3),
        .prompt_origin = new_Vector2(2, 1),
        .error_origin = new_Vector2(2, 6),
        .footer_origin = new_Vector2(2, 29)
    };

    state->last_view_mode = ctx->curViewMode;
}

static void debug_level_apply_player_physics(DebugLevelState* state, GameContext* ctx)
{
    if (state == NULL || ctx == NULL || state->player == NULL || state->player->sprite == NULL)
        return;

    Player* player = state->player;

    if (!player->alive)
        return;

    bool supported_before = physics_body_has_support(
        &state->collision_map,
        player->position,
        player->sprite->size
    );

    if (!supported_before)
        player->grounded = false;

    if (!player->grounded)
        player->velocity.y += 1;

    PhysicsMoveResult result = physics_resolve_motion(
        &state->collision_map,
        player->position,
        player->sprite->size,
        player->velocity,
        player,
        ctx,
        debug_level_player_trigger_proxy
    );

    player->position = result.position;
    player->sprite->position = player->position;

    if (result.blocked_x)
        player->velocity.x = 0;

    bool supported_after = physics_body_has_support(
        &state->collision_map,
        player->position,
        player->sprite->size
    );

    if (result.blocked_y || supported_after)
    {
        player->grounded = true;
        player->velocity.y = 0;
    }
    else if (player->velocity.y != 0)
    {
        player->grounded = false;
    }

    if (player->state != PLAYER_STATE_HIT && player->state != PLAYER_STATE_DIE)
    {
        if (player->grounded)
        {
            if (player->velocity.x != 0)
                player_apply_state(player, PLAYER_STATE_WALK);
            else
                player_apply_state(player, PLAYER_STATE_IDLE);
        }
        else if (player->velocity.y < 0)
        {
            player_apply_state(player, PLAYER_STATE_JUMP);
        }
    }
}

static void debug_level_apply_demo_entity_motion(DebugLevelState* state, GameContext* ctx)
{
    if (state == NULL || ctx == NULL || state->demo_entity == NULL)
        return;

    PhysicsMoveResult result = physics_resolve_motion(
        &state->collision_map,
        state->demo_entity->position,
        state->demo_entity->size,
        VETOR_DIREITA,
        state->demo_entity,
        ctx,
        NULL
    );

    state->demo_entity->position = result.position;
    if (state->demo_entity->representation != NULL)
        state->demo_entity->representation->position = state->demo_entity->position;
}

void debug_level_handle_input(DebugLevelState* state, GameContext* ctx, char input)
{
    if (state == NULL || ctx == NULL)
        return;

    if (input >= '1' && input <= '5')
        ctx->curViewMode = (ViewMode)(input - '1');

    if (ctx->curViewMode != state->last_view_mode)
    {
        limpar_buffer(ctx->curScreen(ctx));

        if (state->last_view_mode == TERMINAL && ctx->curViewMode != TERMINAL)
        {
            clear_codefile_render_area(state->editor_session.editor_codefile, 5, 120);
            codeeditor_prepare_terminal(&state->editor_session);
            codeeditor_reset_editor(&state->editor_session);
        }

        if (ctx->curViewMode == TERMINAL && state->last_view_mode != TERMINAL)
        {
            codeeditor_prepare_terminal(&state->editor_session);
            codeeditor_reset_editor(&state->editor_session);
        }

        state->last_view_mode = ctx->curViewMode;
    }

    if (ctx->curViewMode != TERMINAL)
        return;

    bool terminal_input_changed = false;
    if (input == 'A')
    {
        move_code_cursor(&state->cursor, state->editor_session.editor_codefile, -1, 0);
        terminal_input_changed = true;
    }
    else if (input == 'D')
    {
        move_code_cursor(&state->cursor, state->editor_session.editor_codefile, 1, 0);
        terminal_input_changed = true;
    }
    else if (input == 'W')
    {
        move_code_cursor(&state->cursor, state->editor_session.editor_codefile, 0, -1);
        terminal_input_changed = true;
    }
    else if (input == 'S')
    {
        move_code_cursor(&state->cursor, state->editor_session.editor_codefile, 0, 1);
        terminal_input_changed = true;
    }

    if (input == 'Y' || input == 'N' || input == BACKSPACE_KEY)
    {
        CodeLine* cur_line = state->editor_session.editor_codefile->lines[state->cursor.position.y];
        if (cur_line != NULL && cur_line->size > 0)
        {
            int tk_idx = state->cursor.position.x;
            if (tk_idx >= 0 && tk_idx < cur_line->size)
            {
                CodeToken* tk = &cur_line->tokens[tk_idx];
                if (input == BACKSPACE_KEY)
                {
                    void* bound_target = tk->target_ptr;
                    set_token_literal(tk, "_", TOKEN_NULL, criar_cor(160, 160, 160));
                    tk->target_ptr = bound_target;
                    terminal_input_changed = true;
                }
                else
                {
                    bool val = (input == 'Y');
                    Color color = val ? criar_cor(0, 200, 60) : criar_cor(200, 60, 60);
                    set_token_literal(tk, val ? "true" : "false", TOKEN_BOOL, color);
                    terminal_input_changed = true;
                }
            }
        }
    }

    if (input == 'R')
    {
        terminal_input_changed = true;
        if (codeeditor_try_compile_and_run(&state->editor_session, ctx))
            ctx->curViewMode = VISUAL;
        else
            ctx->curViewMode = TERMINAL;
    }

    if (terminal_input_changed)
        codeeditor_mark_terminal_dirty(&state->editor_session);
}

void debug_level_draw(DebugLevelState* state, GameContext* ctx)
{
    if (state == NULL || ctx == NULL)
        return;

    if (ctx->curViewMode == TERMINAL)
        codeeditor_render_terminal(&state->editor_session, ctx, state->cursor, state->chrome);
    else if (ctx->curViewMode == DEBUG_COLLISION)
    {
        player_draw_view(state->player, ctx);
        ctx->render(ctx);
        physics_draw_collision_matrix(
            &state->collision_map,
            ctx->curScreen(ctx),
            state->player->position,
            state->player->sprite->size,
            true
        );
    }
    else
    {
        player_draw_view(state->player, ctx);
        entity_draw_current_view(state->demo_entity, ctx);
        entity_draw_current_view(state->wall, ctx);
        ctx->render(ctx);
    }
}

void debug_level_tick(DebugLevelState* state, GameContext* ctx)
{
    if (state == NULL || ctx == NULL || ctx->curViewMode == TERMINAL)
        return;

    debug_level_apply_player_physics(state, ctx);

    if (ctx->curViewMode == VISUAL)
        debug_level_apply_demo_entity_motion(state, ctx);
}

void debug_level_destroy(DebugLevelState* state)
{
    if (state == NULL)
        return;

    destroy_player(state->player);
    destroy_entity(state->demo_entity);
    destroy_entity(state->wall);
    collision_matrix_destroy(&state->collision_map);
    codeeditor_destroy(&state->editor_session);
    codebinding_registry_destroy(&state->binding_registry);
}

#endif