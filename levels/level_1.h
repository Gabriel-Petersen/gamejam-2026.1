#ifndef LEVEL_1_H
#define LEVEL_1_H

#include <stdlib.h>
#include "../level.h"

enum {
    LEVEL1_PLATFORM_Y = 24,
    LEVEL1_PLATFORM_VISUAL_OFFSET_Y = 1,
    LEVEL1_GOAL_VISUAL_OFFSET_Y = -4,
    LEVEL1_LEFT_PLATFORM_START = 8,
    LEVEL1_LEFT_PLATFORM_END = 31,
    LEVEL1_BRIDGE_START = 32,
    LEVEL1_BRIDGE_END = 75,
    LEVEL1_RIGHT_PLATFORM_START = 76,
    LEVEL1_RIGHT_PLATFORM_END = 113,
    LEVEL1_GOAL_X = 111,
    LEVEL1_GOAL_Y = 23,
    LEVEL1_RESET_ROW = 31
};

typedef struct Level1State {
    LevelState* common;
    int left_platform_index;
    int right_platform_index;
    int bridge_index;
    int exit_portal_index;
    Objeto* level_label;
    bool pending_compile_transition;
    bool bridge_open;
} Level1State;

static Vector2 level1_cell_world(LevelState* common, int cell_x, int cell_y)
{
    return collision_matrix_cell_world(&common->collision_map, cell_x, cell_y);
}

static Vector2 level1_cell_center_world(LevelState* common, int start_x, int end_x, int cell_y)
{
    Vector2 left = level1_cell_world(common, start_x, cell_y);
    Vector2 right = level1_cell_world(common, end_x, cell_y);
    return new_Vector2((left.x + right.x) / 2, left.y);
}

static void level1_draw_entity_if_present(LevelState* common, GameContext* ctx, int index)
{
    Entity* entity = level_state_entity_at(common, index);
    if (entity != NULL)
        entity_draw_current_view(entity, ctx);
}

static bool level1_is_reset_trigger(Vector2 cell_world_pos)
{
    return cell_world_pos.y == LEVEL1_RESET_ROW;
}

static void level1_reset_player(Player* player)
{
    if (player == NULL)
        return;

    LevelState* common = (LevelState*)player->user_data;
    if (common == NULL)
        return;

    level_state_reset_player(common);
    player_apply_state(player, PLAYER_STATE_IDLE);
}

static void level1_player_on_triggering(Player* player, uint8_t cell_type, Vector2 cell_pos, GameContext* ctx)
{
    if (player == NULL || cell_type != COLLISION_CELL_TRIGGER)
        return;

    if (level1_is_reset_trigger(cell_pos))
        level1_reset_player(player);
    else
        player_apply_state(player, PLAYER_STATE_HIT);

    (void)ctx;
}

static void level1_player_trigger_proxy(void* actor, uint8_t cell_type, Vector2 cell_pos, GameContext* ctx)
{
    Player* player = (Player*)actor;
    if (player == NULL || player->on_triggering == NULL)
        return;

    player->on_triggering(player, cell_type, cell_pos, ctx);
}

static void level1_fill_floor_segment(CollisionMatrix* matrix, int start_x, int end_x, int y, uint8_t value)
{
    if (matrix == NULL)
        return;

    for (int x = start_x; x <= end_x; x++)
        collision_matrix_set(matrix, x, y, value);
}

static ObjetoComplexo* level1_create_platform_visual(Color color, Vector2 size)
{
    Objeto** frames = (Objeto**)malloc(sizeof(Objeto*));
    frames[0] = criar_retangulo_monocromatico(color, size);
    frames[0]->position = VETOR_NULO;
    ObjetoComplexo* representation = criar_objeto_complexo_via_lista(frames, 1);
    centralizar_objeto_complexo(representation);
    return representation;
}

static void level1_setup_collision_map(LevelState* common)
{
    if (common == NULL)
        return;

    collision_matrix_init(
        &common->collision_map,
        SCREEN_SIZE_X,
        SCREEN_SIZE_Y,
        new_Vector2(-(SCREEN_SIZE_X / 2), -(SCREEN_SIZE_Y / 2))
    );
    collision_matrix_fill(&common->collision_map, COLLISION_CELL_FREE);

    level1_fill_floor_segment(&common->collision_map, LEVEL1_LEFT_PLATFORM_START, LEVEL1_LEFT_PLATFORM_END, LEVEL1_PLATFORM_Y, COLLISION_CELL_SOLID);
    level1_fill_floor_segment(&common->collision_map, LEVEL1_RIGHT_PLATFORM_START, LEVEL1_RIGHT_PLATFORM_END, LEVEL1_PLATFORM_Y, COLLISION_CELL_SOLID);

    for (int x = 0; x < common->collision_map.width; x++)
        collision_matrix_set(&common->collision_map, x, LEVEL1_RESET_ROW, COLLISION_CELL_TRIGGER);

    collision_matrix_set(&common->collision_map, LEVEL1_GOAL_X, LEVEL1_GOAL_Y, 3);
}

static void level1_set_bridge_state(LevelState* common, Level1State* local, bool bridge_open)
{
    if (common == NULL || local == NULL)
        return;

    uint8_t bridge_value = bridge_open ? COLLISION_CELL_SOLID : COLLISION_CELL_FREE;
    level1_fill_floor_segment(&common->collision_map, LEVEL1_BRIDGE_START, LEVEL1_BRIDGE_END, LEVEL1_PLATFORM_Y, bridge_value);

    Entity* bridge = level_state_entity_at(common, local->bridge_index);
    if (bridge != NULL)
    {
        bridge->position = level1_cell_center_world(common, LEVEL1_BRIDGE_START, LEVEL1_BRIDGE_END, LEVEL1_PLATFORM_Y);
        bridge->position.y += LEVEL1_PLATFORM_VISUAL_OFFSET_Y;
        bridge->size = new_Vector2(LEVEL1_BRIDGE_END - LEVEL1_BRIDGE_START + 1, 2);
        if (bridge->representation != NULL)
            bridge->representation->position = bridge->position;
        entity_set_visible(bridge, bridge_open);
        bridge->collidable = bridge_open;
    }

    local->bridge_open = bridge_open;
}

static void level1_bridge_on_token(Entity* bridge, CodeToken* token, GameContext* ctx)
{
    if (bridge == NULL || token == NULL || ctx == NULL)
        return;

    Level1State* local = (Level1State*)bridge->user_data;
    if (local == NULL || local->common == NULL)
        return;

    if (token->type == TOKEN_NULL)
        return;

    bool bridge_open = (strcmp(token->string, "true") == 0 || strcmp(token->string, "1") == 0);
    level1_set_bridge_state(local->common, local, bridge_open);
}

static bool level1_setup_bindings(LevelState* common, Level1State* local, Entity* bridge)
{
    if (common == NULL || local == NULL || bridge == NULL)
        return false;

    bridge->user_data = local;
    return codebinding_registry_add(&common->binding_registry, bridge, 0, 4, level1_bridge_on_token);
}

static CodeFile* create_level1_codefile(void)
{
    CodeFile* file = create_codefile();
    set_codefile_terminal_prompt(file, "codeworld@level1:/bridge/door$ edit bridge_is_open.cpp");
    set_codefile_compile_command(file, "g++ bridge_is_open.cpp -O3 -o bridge_is_open && ./bridge_is_open");

    CodeLine* line = push_back_empty_codeline(file);
    push_immutable_token(line, "const", criar_cor(220, 200, 90));
    push_immutable_token(line, "bool", criar_cor(90, 220, 110));
    push_immutable_token(line, "bridge_is_open", criar_cor(90, 150, 230));
    push_immutable_token(line, "=", criar_cor(220, 90, 90));
    push_immutable_token(line, "false", criar_cor(180, 120, 220));
        line->tokens[line->size - 1].type = TOKEN_BOOL;
        line->tokens[line->size - 1].slot_type = TOKEN_BOOL;
    push_immutable_token(line, ";", criar_cor(220, 200, 90));

    line = push_back_empty_codeline(file);
    push_immutable_token(line, "if", criar_cor(90, 220, 110));
    push_immutable_token(line, "(", criar_cor(220, 200, 90));
    push_immutable_token(line, "bridge_is_open", criar_cor(90, 150, 230));
    push_immutable_token(line, ")", criar_cor(220, 200, 90));
    push_immutable_token(line, "{", criar_cor(220, 90, 90));

    line = push_back_empty_codeline(file);
    push_indent_token(line, 4);
    push_immutable_token(line, "raise_bridge", criar_cor(90, 150, 230));
    push_immutable_token(line, "();", criar_cor(220, 200, 90));

    line = push_back_empty_codeline(file);
    push_immutable_token(line, "}", criar_cor(220, 90, 90));

    return file;
}

static bool level1_init(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL || ctx == NULL || ctx->game == NULL)
        return false;

    LevelState* common = &level->common;
    Level1State* local = (Level1State*)malloc(sizeof(Level1State));
    if (local == NULL)
        return false;

    level->state = local;
    local->common = common;
    local->left_platform_index = -1;
    local->right_platform_index = -1;
    local->bridge_index = -1;
    local->exit_portal_index = -1;
    local->level_label = NULL;
    local->pending_compile_transition = false;
    local->bridge_open = false;

    level_state_init(common, ctx->game->curViewMode);
    level1_setup_collision_map(common);

    common->player = create_player(new_Vector2(-38, -4));
    if (common->player == NULL)
    {
        free(local);
        level->state = NULL;
        return false;
    }

    common->player->user_data = common;
    common->player->on_triggering = level1_player_on_triggering;
    common->player_spawn = common->player->position;

    codebinding_registry_init(&common->binding_registry);
    codeeditor_init(&common->editor_session, create_level1_codefile());
    codeeditor_set_bindings(&common->editor_session, &common->binding_registry);

    Entity* left_platform = create_entity(
        level1_cell_center_world(common, LEVEL1_LEFT_PLATFORM_START, LEVEL1_LEFT_PLATFORM_END, LEVEL1_PLATFORM_Y),
        &((Vector2){LEVEL1_LEFT_PLATFORM_END - LEVEL1_LEFT_PLATFORM_START + 1, 2}),
        NULL
    );
    Entity* right_platform = create_entity(
        level1_cell_center_world(common, LEVEL1_RIGHT_PLATFORM_START, LEVEL1_RIGHT_PLATFORM_END, LEVEL1_PLATFORM_Y),
        &((Vector2){LEVEL1_RIGHT_PLATFORM_END - LEVEL1_RIGHT_PLATFORM_START + 1, 2}),
        NULL
    );
    Entity* bridge = create_entity(
        level1_cell_center_world(common, LEVEL1_BRIDGE_START, LEVEL1_BRIDGE_END, LEVEL1_PLATFORM_Y),
        &((Vector2){LEVEL1_BRIDGE_END - LEVEL1_BRIDGE_START + 1, 2}),
        NULL
    );
    Entity* exit_portal = create_entity(
        level1_cell_world(common, LEVEL1_GOAL_X, LEVEL1_GOAL_Y),
        &((Vector2){5, 8}),
        NULL
    );
    if (left_platform != NULL)
        left_platform->position.y += LEVEL1_PLATFORM_VISUAL_OFFSET_Y;
    if (right_platform != NULL)
        right_platform->position.y += LEVEL1_PLATFORM_VISUAL_OFFSET_Y;
    if (bridge != NULL)
        bridge->position.y += LEVEL1_PLATFORM_VISUAL_OFFSET_Y;
    if (exit_portal != NULL)
        exit_portal->position.y += LEVEL1_GOAL_VISUAL_OFFSET_Y;

    if (left_platform == NULL || right_platform == NULL || bridge == NULL || exit_portal == NULL)
    {
        destroy_entity(left_platform);
        destroy_entity(right_platform);
        destroy_entity(bridge);
        destroy_entity(exit_portal);
        level_state_destroy(common);
        free(local);
        level->state = NULL;
        return false;
    }

    entity_attach_representation(
        left_platform,
        level1_create_platform_visual(criar_cor(30, 60, 140), nv2(left_platform->size.x, left_platform->size.y)),
        true
    );
    entity_attach_representation(
        right_platform,
        level1_create_platform_visual(criar_cor(30, 60, 140), nv2(right_platform->size.x, right_platform->size.y)),
        true
    );
    entity_attach_representation(
        bridge,
        level1_create_platform_visual(criar_cor(220, 200, 50), nv2(bridge->size.x, bridge->size.y)),
        true
    );
    entity_attach_representation(
        exit_portal,
        level1_create_platform_visual(criar_cor(80, 220, 90), nv2(exit_portal->size.x, exit_portal->size.y)),
        true
    );

    local->level_label = criar_objeto_de_texto(1, 1, "Level 1");
    if (local->level_label != NULL)
    {
        centralizar_objeto(local->level_label);
        local->level_label->position = new_Vector2(-49, -13);
    }

    entity_set_visible(left_platform, true);
    entity_set_visible(right_platform, true);
    entity_set_visible(bridge, false);
    entity_set_visible(exit_portal, true);

    left_platform->collidable = false;
    right_platform->collidable = false;
    bridge->collidable = false;
    exit_portal->collidable = false;

    if (!level_state_add_entity(common, left_platform, &local->left_platform_index)
        || !level_state_add_entity(common, right_platform, &local->right_platform_index)
        || !level_state_add_entity(common, bridge, &local->bridge_index)
        || !level_state_add_entity(common, exit_portal, &local->exit_portal_index))
    {
        destroy_entity(left_platform);
        destroy_entity(right_platform);
        destroy_entity(bridge);
        destroy_entity(exit_portal);
        if (local->level_label != NULL)
            excluir_objeto(local->level_label);
        level_state_destroy(common);
        free(local);
        level->state = NULL;
        return false;
    }

    if (!level1_setup_bindings(common, local, bridge))
    {
        level_state_destroy(common);
        free(local);
        level->state = NULL;
        return false;
    }

    common->cursor = (CodeCursor){
        .position = VETOR_NULO,
        .bg_color = criar_cor(25, 240, 255),
        .imm_bg_color = criar_cor(255, 235, 40)
    };

    common->chrome = (CodeEditorChrome){
        .code_origin = new_Vector2(2, 3),
        .prompt_origin = new_Vector2(2, 1),
        .error_origin = new_Vector2(2, 25),
        .footer_origin = new_Vector2(2, 29)
    };

    common->last_view_mode = ctx->game->curViewMode;
    level1_set_bridge_state(common, local, false);

    return true;
}

static void level1_handle_input(LevelInstance* level, LevelContext* ctx, char input)
{
    if (level == NULL || ctx == NULL)
        return;

    LevelState* common = &level->common;
    Level1State* local = (Level1State*)level->state;

    if (input >= '1' && input <= '5')
        ctx->game->curViewMode = (ViewMode)(input - '1');

    if (ctx->game->curViewMode != common->last_view_mode)
    {
        limpar_buffer(ctx->game->curScreen(ctx->game));
        if (common->last_view_mode == TERMINAL && ctx->game->curViewMode != TERMINAL)
        {
            clear_codefile_render_area(common->editor_session.editor_codefile, 5, 120);
            codeeditor_prepare_terminal(&common->editor_session);
            codeeditor_reset_editor(&common->editor_session);
        }

        if (ctx->game->curViewMode == TERMINAL && common->last_view_mode != TERMINAL)
        {
            codeeditor_prepare_terminal(&common->editor_session);
            codeeditor_reset_editor(&common->editor_session);
        }

        common->last_view_mode = ctx->game->curViewMode;
    }

    if (ctx->game->curViewMode != TERMINAL)
        return;

    bool terminal_input_changed = false;
    if (input == 'T')
    {
        codeeditor_handle_text_submission_shortcut(&common->editor_session, common->cursor, input);
        terminal_input_changed = true;
    }
    if (input == 'A')
    {
        move_code_cursor(&common->cursor, common->editor_session.editor_codefile, -1, 0);
        codeeditor_mark_terminal_cursor_dirty(&common->editor_session);
        terminal_input_changed = false;
    }
    else if (input == 'D')
    {
        move_code_cursor(&common->cursor, common->editor_session.editor_codefile, 1, 0);
        codeeditor_mark_terminal_cursor_dirty(&common->editor_session);
        terminal_input_changed = false;
    }
    else if (input == 'W')
    {
        move_code_cursor(&common->cursor, common->editor_session.editor_codefile, 0, -1);
        codeeditor_mark_terminal_cursor_dirty(&common->editor_session);
        terminal_input_changed = false;
    }
    else if (input == 'S')
    {
        move_code_cursor(&common->cursor, common->editor_session.editor_codefile, 0, 1);
        codeeditor_mark_terminal_cursor_dirty(&common->editor_session);
        terminal_input_changed = false;
    }

    if (input == BACKSPACE_KEY)
    {
        CodeLine* cur_line = common->editor_session.editor_codefile->lines[common->cursor.position.y];
        if (cur_line != NULL && cur_line->size > 0)
        {
            int tk_idx = common->cursor.position.x;
            if (tk_idx >= 0 && tk_idx < cur_line->size)
            {
                CodeToken* tk = &cur_line->tokens[tk_idx];
                if (tk->slot_type == TOKEN_IMMUTABLE || tk->type == TOKEN_INDENT)
                {
                    codeeditor_set_text_hint(&common->editor_session, "Locked token: only editable slots can be cleared.");
                    terminal_input_changed = true;
                    goto level1_terminal_input_done;
                }
                void* bound_target = tk->target_ptr;
                set_token_literal(tk, "_", TOKEN_NULL, criar_cor(160, 160, 160));
                tk->target_ptr = bound_target;
                terminal_input_changed = true;
            }
        }
    }

    if (input == 'R')
    {
        terminal_input_changed = true;
        if (codeeditor_try_compile_and_run(&common->editor_session, ctx->game))
        {
            local->pending_compile_transition = true;
            ctx->game->curViewMode = TERMINAL;
        }
        else
            ctx->game->curViewMode = TERMINAL;
    }

level1_terminal_input_done:
    if (terminal_input_changed)
        codeeditor_mark_terminal_dirty(&common->editor_session);
}

static void level1_tick(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL || ctx == NULL || ctx->game->curViewMode == TERMINAL)
        return;

    LevelState* common = &level->common;

    if (common->player != NULL)
        player_update(common->player, ctx->game, ctx->last_input);

    if (common->player != NULL)
    {
        Player* player = common->player;

        if (!player->alive)
            return;

        bool supported_before = physics_body_has_support(
            &common->collision_map,
            player->position,
            player->sprite->size
        );

        if (!supported_before)
            player->grounded = false;

        if (!player->grounded)
            player->velocity.y += 1;

        PhysicsMoveResult result = physics_resolve_motion(
            &common->collision_map,
            player->position,
            player->sprite->size,
            player->velocity,
            player,
            ctx->game,
            level1_player_trigger_proxy
        );

        player->position = result.position;
        player->sprite->position = player->position;

        if (result.blocked_x)
            player->velocity.x = 0;

        bool supported_after = physics_body_has_support(
            &common->collision_map,
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

    level_state_resolve_player_entity_collisions(common);

    if (level_state_player_overlaps_cell_type(common, COLLISION_CELL_TRIGGER))
    {
        level1_reset_player(common->player);
        return;
    }

    if (level_state_player_overlaps_cell_type(common, 3))
        level_request_exit(level, LEVEL_EXIT_COMPLETED, 1);
}

static void level1_draw(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL || ctx == NULL)
        return;

    LevelState* common = &level->common;
    Level1State* local = (Level1State*)level->state;

    if (ctx->game->curViewMode == TERMINAL)
    {
        codeeditor_render_terminal(&common->editor_session, ctx->game, common->cursor, common->chrome);

        if (local != NULL && local->pending_compile_transition)
        {
            esperar(1000);
            ctx->game->curViewMode = VISUAL;
            limpar_buffer(ctx->game->curScreen(ctx->game));
            clear_codefile_compile_success_message(common->editor_session.editor_codefile);
            local->pending_compile_transition = false;
            common->last_view_mode = VISUAL;
        }

        return;
    }

    if (ctx->game->curViewMode == VISUAL && local != NULL)
    {
        if (local->level_label != NULL)
            desenhar_objeto(ctx->game->curScreen(ctx->game), local->level_label);
    }

    if (ctx->game->curViewMode == DEBUG_STATE && local != NULL)
    {
        level1_draw_entity_if_present(common, ctx->game, local->left_platform_index);
        level1_draw_entity_if_present(common, ctx->game, local->right_platform_index);
        level1_draw_entity_if_present(common, ctx->game, local->exit_portal_index);

        player_draw_view(common->player, ctx->game);
        ctx->game->render(ctx->game);

        print_rgb_txt_bg(
            criar_cor(255, 245, 180),
            criar_cor(20, 24, 34),
            new_Vector2(39, 25),
            "bridge.enabled = %s;",
            local->bridge_open ? "true" : "false"
        );

        esperar(120);
        player_hide_view(common->player, ctx->game);
        level_state_hide_entities(common, ctx->game);
        return;
    }

    if (ctx->game->curViewMode == DEBUG_COLLISION)
    {
        player_draw_view(common->player, ctx->game);
        level_state_draw_entities(common, ctx->game);
        ctx->game->render(ctx->game);
        physics_draw_collision_matrix(
            &common->collision_map,
            ctx->game->curScreen(ctx->game),
            common->player->position,
            common->player->sprite->size,
            true
        );
    }
    else
    {
        player_draw_view(common->player, ctx->game);
        level_state_draw_entities(common, ctx->game);
        ctx->game->render(ctx->game);
    }

    esperar(120);
    player_hide_view(common->player, ctx->game);
    level_state_hide_entities(common, ctx->game);
}

static void level1_destroy(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL)
        return;

    Level1State* local = (Level1State*)level->state;
    if (local != NULL && local->level_label != NULL)
    {
        if (ctx != NULL && ctx->game != NULL)
            esconder_objeto(ctx->game->curScreen(ctx->game), local->level_label);
        excluir_objeto(local->level_label);
        local->level_label = NULL;
    }

    if (ctx != NULL && ctx->game != NULL)
        level_state_hide_renderables(&level->common, ctx->game);

    level_state_destroy(&level->common);

    free(level->state);
    level->state = NULL;
}

static const LevelDefinition LEVEL_1_DEFINITION = {
    .id = "level_1",
    .display_name = "Level 1 - Bridge Switch",
    .world_size = {SCREEN_SIZE_X, SCREEN_SIZE_Y},
    .allow_camera_move = false,
    .vtable = {
        .init = level1_init,
        .handle_input = level1_handle_input,
        .tick = level1_tick,
        .draw = level1_draw,
        .destroy = level1_destroy
    }
};

static const LevelDefinition* level1_get_definition(void)
{
    return &LEVEL_1_DEFINITION;
}

#endif
