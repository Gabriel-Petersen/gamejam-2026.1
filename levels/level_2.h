#ifndef LEVEL_2_H
#define LEVEL_2_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../level.h"

typedef struct Level2State {
    LevelState* common;
    int left_platform_index;
    int right_platform_index;
    int goal_index;
    int bridge_indices[3];
    Objeto* level_label;
    bool pending_compile_transition;
    bool bridges_enabled;
} Level2State;

enum {
    LEVEL2_PLATFORM_Y = 24,
    LEVEL2_LEFT_PLATFORM_START = 8,
    LEVEL2_LEFT_PLATFORM_END = 37,
    
    LEVEL2_BRIDGE_BASE_START = 47,
    LEVEL2_BRIDGE_SEGMENT_SPACING = 13,
    LEVEL2_BRIDGE_SEGMENT_WIDTH = 3,
    
    LEVEL2_RIGHT_PLATFORM_START = 86,
    LEVEL2_RIGHT_PLATFORM_END = 114,  
    LEVEL2_GOAL_X = 108,              
    LEVEL2_GOAL_Y = 23,
    LEVEL2_RESET_ROW = 31
};

static CodeFile* create_level2_codefile(void)
{
    CodeFile* file = create_codefile();
    set_codefile_terminal_prompt(file, "codeworld@level2:/bridge$ edit hide_all_bridges.cpp");
    set_codefile_compile_command(file, "g++ hide_all_bridges.cpp -O3 -o hide_all_bridges && ./hide_all_bridges");

    CodeLine* line = push_back_empty_codeline(file);
    push_immutable_token(line, "const", criar_cor(220, 200, 90));
    push_immutable_token(line, "int", criar_cor(90, 220, 110));
    push_immutable_token(line, "bridge_is_open", criar_cor(90, 150, 230));
    push_immutable_token(line, "=", criar_cor(220, 90, 90));
    push_immutable_token(line, "false", criar_cor(180, 120, 220));
    line->tokens[line->size - 1].type = TOKEN_BOOL;
    line->tokens[line->size - 1].slot_type = TOKEN_BOOL;
    push_immutable_token(line, ";", criar_cor(220, 200, 90));

    line = push_back_empty_codeline(file);
    push_immutable_token(line, "hide_all_bridges", criar_cor(90, 220, 110));
    push_immutable_token(line, "();", criar_cor(220, 200, 90));

    line = push_back_empty_codeline(file);
    push_immutable_token(line, "if", criar_cor(90, 220, 110));
    push_immutable_token(line, "(", criar_cor(220, 200, 90));
    push_immutable_token(line, "bridge_is_open", criar_cor(90, 150, 230));
    push_immutable_token(line, ")", criar_cor(220, 200, 90));
    push_immutable_token(line, "{", criar_cor(220, 90, 90));

    line = push_back_empty_codeline(file);
    push_indent_token(line, 4);
    push_immutable_token(line, "show_bridge_path", criar_cor(90, 150, 230));
    push_immutable_token(line, "();", criar_cor(220, 200, 90));

    line = push_back_empty_codeline(file);
    push_immutable_token(line, "}", criar_cor(220, 90, 90));

    return file;
}

static ObjetoComplexo* level2_create_platform_visual(Color color, Vector2 size)
{
    Objeto** frames = (Objeto**)malloc(sizeof(Objeto*));
    frames[0] = criar_retangulo_monocromatico(color, size);
    frames[0]->position = VETOR_NULO;
    ObjetoComplexo* representation = criar_objeto_complexo_via_lista(frames, 1);
    return representation;
}

static void level2_fill_floor_segment(CollisionMatrix* matrix, int start_x, int end_x, int y, uint8_t value)
{
    if (matrix == NULL)
        return;

    for (int x = start_x; x <= end_x; x++)
        collision_matrix_set(matrix, x, y, value);
}

static void level2_setup_collision_map(LevelState* common)
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

    level2_fill_floor_segment(&common->collision_map, LEVEL2_LEFT_PLATFORM_START, LEVEL2_LEFT_PLATFORM_END, LEVEL2_PLATFORM_Y, COLLISION_CELL_SOLID);
    level2_fill_floor_segment(&common->collision_map, LEVEL2_RIGHT_PLATFORM_START, LEVEL2_RIGHT_PLATFORM_END, LEVEL2_PLATFORM_Y, COLLISION_CELL_SOLID);

    collision_matrix_set(&common->collision_map, LEVEL2_GOAL_X, LEVEL2_GOAL_Y, 3);

    /* reset row: falling into this row resets player to spawn */
    for (int x = 0; x < common->collision_map.width; x++)
        collision_matrix_set(&common->collision_map, x, LEVEL2_RESET_ROW, COLLISION_CELL_TRIGGER);
}

static bool level2_is_reset_trigger(Vector2 cell_world_pos)
{
    return cell_world_pos.y == LEVEL2_RESET_ROW;
}

static void level2_reset_player(Player* player)
{
    if (player == NULL) return;
    LevelState* common = (LevelState*)player->user_data;
    if (common == NULL) return;
    level_state_reset_player(common);
    player->velocity = VETOR_NULO; 
    player->grounded = true;
    player_apply_state(player, PLAYER_STATE_IDLE);
}

static void level2_player_trigger_proxy(void* actor, uint8_t cell_type, Vector2 cell_pos, GameContext* ctx)
{
    Player* player = (Player*)actor;
    if (player == NULL || player->on_triggering == NULL)
        return;

    player->on_triggering(player, cell_type, cell_pos, ctx);
}

static void level2_player_on_triggering(Player* player, uint8_t cell_type, Vector2 cell_pos, GameContext* ctx)
{
    if (player == NULL || cell_type != COLLISION_CELL_TRIGGER)
        return;

    if (level2_is_reset_trigger(cell_pos))
        level2_reset_player(player);
    else
        player_apply_state(player, PLAYER_STATE_HIT);

    (void)ctx;
}

static void level2_sync_bridge_visibility(LevelState* common, Level2State* local, GameContext* game)
{
    if (common == NULL || local == NULL)
        return;
    if (game == NULL) return;

    bool show_bridge = local->bridges_enabled && game->curViewMode == DEBUG_COLLISION;
    for (int i = 0; i < 3; i++)
    {
        Entity* bridge = level_state_entity_at(common, local->bridge_indices[i]);
        if (bridge == NULL)
            continue;

        if (!show_bridge && bridge->representation != NULL && bridge->representation->renderizado)
            entity_hide_representation(bridge, game);

        entity_set_visible(bridge, show_bridge);
        bridge->collidable = local->bridges_enabled;
    }
}

static void level2_set_bridge_state(LevelState* common, Level2State* local, bool bridges_enabled)
{
    if (common == NULL || local == NULL)
        return;

    local->bridges_enabled = bridges_enabled;

    for (int i = 0; i < 3; i++)
    {
        int start_x = LEVEL2_BRIDGE_BASE_START + (i * LEVEL2_BRIDGE_SEGMENT_SPACING);
        int end_x = start_x + LEVEL2_BRIDGE_SEGMENT_WIDTH - 1;
        level2_fill_floor_segment(
            &common->collision_map,
            start_x,
            end_x,
            LEVEL2_PLATFORM_Y,
            bridges_enabled ? COLLISION_CELL_SOLID : COLLISION_CELL_FREE
        );

        Entity* bridge = level_state_entity_at(common, local->bridge_indices[i]);
        if (bridge != NULL)
        {
            bridge->collidable = bridges_enabled;
            if (bridge->representation != NULL)
                bridge->representation->position = bridge->position;
        }
    }
}

static void level2_bridge_on_token(Entity* bridge, CodeToken* token, GameContext* ctx)
{
    if (bridge == NULL || token == NULL || ctx == NULL)
        return;

    Level2State* local = (Level2State*)bridge->user_data;
    if (local == NULL || local->common == NULL)
        return;

    if (token->type == TOKEN_NULL)
        return;

    bool open = (strcmp(token->string, "true") == 0 || strcmp(token->string, "1") == 0);
    level2_set_bridge_state(local->common, local, open);
    level2_sync_bridge_visibility(local->common, local, ctx);
}

static bool level2_init(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL || ctx == NULL || ctx->game == NULL)
        return false;

#ifndef DEBUG_ENABLE
    DEBUG_LOG("[L2] init begin\n");
#endif

    LevelState* common = &level->common;
    Level2State* local = (Level2State*)malloc(sizeof(Level2State));
    if (local == NULL)
    {
#ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] init fail: local allocation failed\n");
#endif
        return false;
    }

#ifndef DEBUG_ENABLE
    DEBUG_LOG("[L2] init local=%p common=%p\n", (void*)local, (void*)common);
#endif

    level->state = local;
    local->common = common;
    local->left_platform_index = -1;
    local->right_platform_index = -1;
    local->goal_index = -1;
    for (int i = 0; i < 3; i++)
        local->bridge_indices[i] = -1;
    local->level_label = NULL;
    local->pending_compile_transition = false;
    local->bridges_enabled = false;

    level_state_init(common, ctx->game->curViewMode);
#ifndef DEBUG_ENABLE
    DEBUG_LOG("[L2] init after level_state_init\n");
#endif
    game_clear_all_screens(ctx->game);

    level2_setup_collision_map(common);
#ifndef DEBUG_ENABLE
    DEBUG_LOG("[L2] init collision map ready width=%d height=%d cells=%p\n", common->collision_map.width, common->collision_map.height, (void*)common->collision_map.cells);
#endif

    codebinding_registry_init(&common->binding_registry);
    codeeditor_init(&common->editor_session, create_level2_codefile());
    codeeditor_set_bindings(&common->editor_session, &common->binding_registry);
#ifndef DEBUG_ENABLE
    DEBUG_LOG("[L2] init editor ready editor=%p committed=%p bindings=%p\n",
        (void*)common->editor_session.editor_codefile,
        (void*)common->editor_session.committed_codefile,
        (void*)&common->binding_registry);
#endif

    Entity* left_platform = create_entity(
        collision_matrix_cell_world(&common->collision_map, LEVEL2_LEFT_PLATFORM_START, LEVEL2_PLATFORM_Y + 1),
        &((Vector2){LEVEL2_LEFT_PLATFORM_END - LEVEL2_LEFT_PLATFORM_START + 1, 2}),
        NULL
    );
    Entity* right_platform = create_entity(
        collision_matrix_cell_world(&common->collision_map, LEVEL2_RIGHT_PLATFORM_START, LEVEL2_PLATFORM_Y + 1),
        &((Vector2){LEVEL2_RIGHT_PLATFORM_END - LEVEL2_RIGHT_PLATFORM_START + 1, 2}),
        NULL
    );
    Entity* goal_portal = create_entity(
        vector_sum(
            v_prod(VETOR_CIMA, 8), 
            collision_matrix_cell_world(&common->collision_map, LEVEL2_GOAL_X, LEVEL2_GOAL_Y)
        ),
        &((Vector2){5, 8}),
        NULL
    );
    Entity* bridges[3] = {NULL, NULL, NULL};

    if (left_platform == NULL || right_platform == NULL || goal_portal == NULL)
    {
    #ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] init fail: base entities left=%p right=%p goal=%p\n", (void*)left_platform, (void*)right_platform, (void*)goal_portal);
    #endif
        destroy_entity(left_platform);
        destroy_entity(right_platform);
        destroy_entity(goal_portal);
        level_state_destroy(common);
        free(local);
        level->state = NULL;
        return false;
    }

    entity_attach_representation(
        left_platform,
        level2_create_platform_visual(criar_cor(30, 60, 140), nv2(left_platform->size.x, left_platform->size.y)),
        true
    );
    entity_attach_representation(
        right_platform,
        level2_create_platform_visual(criar_cor(30, 60, 140), nv2(right_platform->size.x, right_platform->size.y)),
        true
    );
    entity_attach_representation(
        goal_portal,
        level2_create_platform_visual(criar_cor(80, 220, 90), nv2(goal_portal->size.x, goal_portal->size.y)),
        true
    );

    entity_set_visible(left_platform, true);
    entity_set_visible(right_platform, true);
    entity_set_visible(goal_portal, true);

    for (int i = 0; i < 3; i++)
    {
        int bridge_start_x = LEVEL2_BRIDGE_BASE_START + (i * LEVEL2_BRIDGE_SEGMENT_SPACING);
        int bridge_end_x = bridge_start_x + LEVEL2_BRIDGE_SEGMENT_WIDTH - 1;
        bridges[i] = create_entity(
            collision_matrix_cell_world(&common->collision_map, bridge_start_x, LEVEL2_PLATFORM_Y),
            &((Vector2){bridge_end_x - bridge_start_x + 1, 2}),
            NULL
        );
        if (bridges[i] == NULL)
            continue;

        entity_attach_representation(
            bridges[i],
            level2_create_platform_visual(criar_cor(220, 200, 50), nv2(bridges[i]->size.x, bridges[i]->size.y)),
            true
        );
        bridges[i]->collidable = false;
        entity_set_visible(bridges[i], false);

        int bridge_index = -1;
        if (!level_state_add_entity(common, bridges[i], &bridge_index))
        {
#ifndef DEBUG_ENABLE
            DEBUG_LOG("[L2] init bridge[%d] add entity failed\n", i);
#endif
            destroy_entity(bridges[i]);
            bridges[i] = NULL;
            continue;
        }

        local->bridge_indices[i] = bridge_index;
        bridges[i]->user_data = local;
#ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] init bridge[%d] entity=%p index=%d pos=(%d,%d) size=(%d,%d) visible=%d collidable=%d\n",
            i,
            (void*)bridges[i],
            bridge_index,
            bridges[i]->position.x,
            bridges[i]->position.y,
            bridges[i]->size.x,
            bridges[i]->size.y,
            bridges[i]->visible,
            bridges[i]->collidable);
#endif
    }

    int left_index = -1;
    int right_index = -1;
    int goal_index = -1;
    if (!level_state_add_entity(common, left_platform, &left_index)
        || !level_state_add_entity(common, right_platform, &right_index)
        || !level_state_add_entity(common, goal_portal, &goal_index))
    {
    #ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] init fail: add base entities left=%d right=%d goal=%d\n", left_index, right_index, goal_index);
    #endif
        destroy_entity(left_platform);
        destroy_entity(right_platform);
        destroy_entity(goal_portal);
        for (int i = 0; i < 3; i++)
            destroy_entity(bridges[i]);
        level_state_destroy(common);
        free(local);
        level->state = NULL;
        return false;
    }

    local->left_platform_index = left_index;
    local->right_platform_index = right_index;
    local->goal_index = goal_index;

#ifndef DEBUG_ENABLE
    DEBUG_LOG("[L2] init base entities left=%p idx=%d right=%p idx=%d goal=%p idx=%d\n",
        (void*)left_platform, left_index,
        (void*)right_platform, right_index,
        (void*)goal_portal, goal_index);
#endif

#ifndef DEBUG_ENABLE
    {
        Vector2 cell_left = collision_matrix_cell_world(&common->collision_map, LEVEL2_LEFT_PLATFORM_START, LEVEL2_PLATFORM_Y);
        Vector2 cell_right = collision_matrix_cell_world(&common->collision_map, LEVEL2_LEFT_PLATFORM_END, LEVEL2_PLATFORM_Y);
        DEBUG_LOG("[L2] left platform cell start=(%d,%d) world_start=(%d,%d) cell_end=(%d,%d) world_end=(%d,%d)\n",
            LEVEL2_LEFT_PLATFORM_START, LEVEL2_PLATFORM_Y,
            cell_left.x, cell_left.y,
            LEVEL2_LEFT_PLATFORM_END, LEVEL2_PLATFORM_Y,
            cell_right.x, cell_right.y);
        if (left_platform->representation) {
            DEBUG_LOG("[L2] left rep pos=(%d,%d) ent pos=(%d,%d) rep_size=(%d,%d) pivot=(%d,%d)\n",
                left_platform->representation->position.x, left_platform->representation->position.y,
                left_platform->position.x, left_platform->position.y,
                left_platform->representation->size.x, left_platform->representation->size.y,
                left_platform->representation->pivot_frames[0].x, left_platform->representation->pivot_frames[0].y);
        }
    }
#endif

    bridges[0]->user_data = local;
    codebinding_registry_add(&common->binding_registry, bridges[0], 0, 4, level2_bridge_on_token);
#ifndef DEBUG_ENABLE
    DEBUG_LOG("[L2] init binding set bridge0=%p token=(0,4)\n", (void*)bridges[0]);
#endif

    local->level_label = criar_objeto_de_texto(1, 1, "Level 2");
    if (local->level_label != NULL)
    {
        centralizar_objeto(local->level_label);
        local->level_label->position = new_Vector2(-49, -13);
    }
#ifndef DEBUG_ENABLE
    DEBUG_LOG("[L2] init label=%p pos=(%d,%d)\n",
        (void*)local->level_label,
        local->level_label != NULL ? local->level_label->position.x : 0,
        local->level_label != NULL ? local->level_label->position.y : 0);
#endif

    if (common->player != NULL)
    {
    #ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] init destroying residual player=%p\n", (void*)common->player);
    #endif
        destroy_player(common->player);
        common->player = NULL;
    }

    common->player = create_player(new_Vector2(-38, -4));
    if (common->player == NULL)
    {
    #ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] init fail: create_player returned NULL\n");
    #endif
        level_state_destroy(common);
        free(local);
        level->state = NULL;
        return false;
    }

    common->player->user_data = common;
    common->player->on_triggering = level2_player_on_triggering;
    common->player_spawn = common->player->position;

#ifndef DEBUG_ENABLE
    DEBUG_LOG("[L2] init player=%p sprite=%p pos=(%d,%d) spawn=(%d,%d)\n",
        (void*)common->player,
        (void*)common->player->sprite,
        common->player->position.x,
        common->player->position.y,
        common->player_spawn.x,
        common->player_spawn.y);
#endif

    level2_set_bridge_state(common, local, false);
    level2_sync_bridge_visibility(common, local, ctx->game);
#ifndef DEBUG_ENABLE
    DEBUG_LOG("[L2] init bridge state initialized enabled=%d\n", local->bridges_enabled);
#endif

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
#ifndef DEBUG_ENABLE
    DEBUG_LOG("[L2] init complete\n");
#endif
    return true;
}

static void level2_handle_input(LevelInstance* level, LevelContext* ctx, char input)
{
    if (level == NULL || ctx == NULL)
        return;

    LevelState* common = &level->common;
    Level2State* local = (Level2State*)level->state;

    if (input >= '1' && input <= '5')
        ctx->game->curViewMode = (ViewMode)(input - '1');

    if (ctx->game->curViewMode != common->last_view_mode)
    {
        game_clear_all_screens(ctx->game);
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

    if (input == 'Q')
        level_request_exit(level, LEVEL_EXIT_QUIT, -1);

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
                    goto level2_terminal_input_done;
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

level2_terminal_input_done:
    if (terminal_input_changed)
        codeeditor_mark_terminal_dirty(&common->editor_session);
}

static void level2_tick(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL || ctx == NULL || ctx->game->curViewMode == TERMINAL)
        return;

    LevelState* common = &level->common;
    Level2State* local = (Level2State*)level->state;

    if (ctx->frame < 3)
    {
#ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] tick frame=%llu mode=%d player=%p entities=%d\n",
            (unsigned long long)ctx->frame,
            (int)ctx->game->curViewMode,
            (void*)common->player,
            common->entity_count);
#endif
    }

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
            level2_player_trigger_proxy
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

    if (level_state_player_overlaps_cell_type(common, 3))
    {
        level_request_exit(level, LEVEL_EXIT_COMPLETED, 2);
        return;
    }

        if (local != NULL && local->bridges_enabled)
            level2_sync_bridge_visibility(common, local, ctx->game);
}

static void level2_draw(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL || ctx == NULL)
        return;
    LevelState* common = &level->common;
    Level2State* local = (Level2State*)level->state;

    if (ctx->frame < 3)
    {
#ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] draw frame=%llu mode=%d player=%p label=%p entities=%d\n",
            (unsigned long long)ctx->frame,
            (int)ctx->game->curViewMode,
            (void*)common->player,
            (void*)(local != NULL ? local->level_label : NULL),
            common->entity_count);
#endif
    }

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

    if (ctx->game->curViewMode == DEBUG_COLLISION)
    {
        level2_sync_bridge_visibility(common, local, ctx->game);

#ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] draw stage=debug-collision before-player\n");
#endif
        if (common->player != NULL)
            player_draw_view(common->player, ctx->game);

#ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] draw stage=debug-collision before-entities\n");
#endif
        level_state_draw_entities(common, ctx->game);

#ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] draw stage=debug-collision before-render\n");
#endif
        ctx->game->render(ctx->game);

#ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] draw stage=debug-collision before-matrix\n");
#endif
        physics_draw_collision_matrix(
            &common->collision_map,
            ctx->game->curScreen(ctx->game),
            common->player != NULL ? common->player->position : VETOR_NULO,
            common->player != NULL ? common->player->sprite->size : nv2(1,1),
            true
        );
    }
    else
    {
        level2_sync_bridge_visibility(common, local, ctx->game);

#ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] draw stage=visual before-player\n");
#endif
        if (common->player != NULL)
            player_draw_view(common->player, ctx->game);

#ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] draw stage=visual before-entities\n");
#endif
        level_state_draw_entities(common, ctx->game);

#ifndef DEBUG_ENABLE
        DEBUG_LOG("[L2] draw stage=visual before-render\n");
#endif
        ctx->game->render(ctx->game);
    }

    esperar(120);

#ifndef DEBUG_ENABLE
    DEBUG_LOG("[L2] draw stage=cleanup hiding views\n");
#endif
    if (common->player != NULL)
        player_hide_view(common->player, ctx->game);
        
    level_state_hide_entities(common, ctx->game);
}

static void level2_destroy(LevelInstance* level, LevelContext* ctx)
{
    return;
    if (level == NULL)
        return;

    Level2State* local = (Level2State*)level->state;
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

static const LevelDefinition LEVEL_2_DEFINITION = {
    .id = "level_2",
    .display_name = "Level 2 - Invisible Bridge",
    .world_size = {SCREEN_SIZE_X, SCREEN_SIZE_Y},
    .allow_camera_move = false,
    .vtable = {
        .init = level2_init,
        .handle_input = level2_handle_input,
        .tick = level2_tick,
        .draw = level2_draw,
        .destroy = level2_destroy
    }
};

static const LevelDefinition* level2_get_definition(void)
{
    return &LEVEL_2_DEFINITION;
}

#endif
