#ifndef LEVEL_3_H
#define LEVEL_3_H

#include <stdlib.h>
#include <string.h>
#include "../level.h"

enum {
    LEVEL3_PLATFORM_Y = 24,
    LEVEL3_PLATFORM_VISUAL_OFFSET_Y = 1,
    LEVEL3_GOAL_VISUAL_OFFSET_Y = -4,
    LEVEL3_FLOOR_START = 5,
    LEVEL3_FLOOR_END = 113,
    LEVEL3_WALL_X_START = 44,
    LEVEL3_WALL_X_END = 46,
    LEVEL3_HARM_X_START = 72,
    LEVEL3_HARM_X_END = 78,
    LEVEL3_GOAL_X = 110,
    LEVEL3_GOAL_Y = 23,
    LEVEL3_RESET_ROW = 31
};

typedef struct Level3State {
    LevelState* common;
    int floor_platform_index;
    int wall_index;
    int harm_block_index;
    int exit_portal_index;
    Objeto* level_label;
    bool pending_compile_transition;
    
    // Estados lógicos controlados pelo código do jogador
    int n_value;
    bool harm_active;
} Level3State;

static Vector2 level3_cell_world(LevelState* common, int cell_x, int cell_y)
{
    return collision_matrix_cell_world(&common->collision_map, cell_x, cell_y);
}

static Vector2 level3_cell_center_world(LevelState* common, int start_x, int end_x, int cell_y)
{
    Vector2 left = level3_cell_world(common, start_x, cell_y);
    Vector2 right = level3_cell_world(common, end_x, cell_y);
    return new_Vector2((left.x + right.x) / 2, left.y);
}

static void level3_draw_entity_if_present(LevelState* common, GameContext* ctx, int index)
{
    Entity* entity = level_state_entity_at(common, index);
    if (entity != NULL)
        entity_draw_current_view(entity, ctx);
}

static void level3_reset_player(Player* player)
{
    if (player == NULL)
        return;

    LevelState* common = (LevelState*)player->user_data;
    if (common == NULL)
        return;

    level_state_reset_player(common);
    player_apply_state(player, PLAYER_STATE_IDLE);
}

static void level3_player_on_triggering(Player* player, uint8_t cell_type, Vector2 cell_pos, GameContext* ctx)
{
    if (player == NULL || cell_type != COLLISION_CELL_TRIGGER)
        return;

    // Qualquer trigger neste level (seja queda ou bloco vermelho ativo) teleporta de volta
    level3_reset_player(player);
    (void)cell_pos;
    (void)ctx;
}

static void level3_player_trigger_proxy(void* actor, uint8_t cell_type, Vector2 cell_pos, GameContext* ctx)
{
    Player* player = (Player*)actor;
    if (player == NULL || player->on_triggering == NULL)
        return;

    player->on_triggering(player, cell_type, cell_pos, ctx);
}

static void level3_fill_floor_segment(CollisionMatrix* matrix, int start_x, int end_x, int y, uint8_t value)
{
    if (matrix == NULL)
        return;

    for (int x = start_x; x <= end_x; x++)
        collision_matrix_set(matrix, x, y, value);
}

static ObjetoComplexo* level3_create_platform_visual(Color color, Vector2 size)
{
    Objeto** frames = (Objeto**)malloc(sizeof(Objeto*));
    frames[0] = criar_retangulo_monocromatico(color, size);
    frames[0]->position = VETOR_NULO;
    ObjetoComplexo* representation = criar_objeto_complexo_via_lista(frames, 1);
    centralizar_objeto_complexo(representation);
    return representation;
}

static void level3_sync_world_state(LevelState* common, Level3State* local)
{
    if (common == NULL || local == NULL)
        return;

    // 1. Sincronizar e limpar área dinâmica da parede
    for (int x = LEVEL3_WALL_X_START; x <= LEVEL3_WALL_X_END; x++)
    {
        for (int y = LEVEL3_PLATFORM_Y - 12; y < LEVEL3_PLATFORM_Y; y++)
        {
            collision_matrix_set(&common->collision_map, x, y, COLLISION_CELL_FREE);
        }
    }

    int current_height = local->n_value;
    if (current_height > 12) current_height = 12;
    if (current_height < 0)  current_height = 0;

    if (current_height > 0)
    {
        for (int x = LEVEL3_WALL_X_START; x <= LEVEL3_WALL_X_END; x++)
        {
            for (int y = LEVEL3_PLATFORM_Y - current_height; y < LEVEL3_PLATFORM_Y; y++)
            {
                collision_matrix_set(&common->collision_map, x, y, COLLISION_CELL_SOLID);
            }
        }
    }

    Entity* wall = level_state_entity_at(common, local->wall_index);
    if (wall != NULL)
    {
        if (current_height > 0)
        {
            Vector2 bottom = level3_cell_center_world(common, LEVEL3_WALL_X_START, LEVEL3_WALL_X_END, LEVEL3_PLATFORM_Y - 1);
            Vector2 top = level3_cell_center_world(common, LEVEL3_WALL_X_START, LEVEL3_WALL_X_END, LEVEL3_PLATFORM_Y - current_height);
            wall->position = new_Vector2((bottom.x + top.x) / 2, (bottom.y + top.y) / 2);
            wall->size = new_Vector2(LEVEL3_WALL_X_END - LEVEL3_WALL_X_START + 1, current_height);
            if (wall->representation != NULL)
                wall->representation->position = wall->position;
            
            entity_set_visible(wall, true);
            wall->collidable = true;
        }
        else
        {
            entity_set_visible(wall, false);
            wall->collidable = false;
        }
    }

    // 2. Sincronizar e reconfigurar a faixa vermelha do Bloco de Dano
    uint8_t harm_cell_value = local->harm_active ? COLLISION_CELL_TRIGGER : COLLISION_CELL_FREE;
    for (int x = LEVEL3_HARM_X_START; x <= LEVEL3_HARM_X_END; x++)
    {
        collision_matrix_set(&common->collision_map, x, LEVEL3_PLATFORM_Y - 1, harm_cell_value);
    }

    Entity* harm = level_state_entity_at(common, local->harm_block_index);
    if (harm != NULL)
    {
        entity_set_visible(harm, local->harm_active);
        harm->collidable = false;
    }
}

static void level3_wall_on_token(Entity* wall, CodeToken* token, GameContext* ctx)
{
    if (wall == NULL || token == NULL)
        return;

    Level3State* local = (Level3State*)wall->user_data;
    if (local == NULL || local->common == NULL)
        return;

    if (token->type == TOKEN_NULL)
        local->n_value = 0;
    else
        local->n_value = atoi(token->string);

    level3_sync_world_state(local->common, local);
    (void)ctx;
}

static void level3_harm_on_token(Entity* harm, CodeToken* token, GameContext* ctx)
{
    if (harm == NULL || token == NULL)
        return;

    Level3State* local = (Level3State*)harm->user_data;
    if (local == NULL || local->common == NULL)
        return;

    if (token->type == TOKEN_NULL)
    {
        local->harm_active = false;
    }
    else
    {
        // Se mudou o operador para < ou <=, a expressão (100 > 0) que era True vira False
        // Ativo apenas se o token explicitamente mantiver o operador de maior '>'
        local->harm_active = (strchr(token->string, '>') != NULL);
    }

    level3_sync_world_state(local->common, local);
    (void)ctx;
}

static void level3_setup_collision_map(LevelState* common)
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

    // Chão base contínuo
    level3_fill_floor_segment(&common->collision_map, LEVEL3_FLOOR_START, LEVEL3_FLOOR_END, LEVEL3_PLATFORM_Y, COLLISION_CELL_SOLID);

    // Trigger de queda livre no fundo
    for (int x = 0; x < common->collision_map.width; x++)
        collision_matrix_set(&common->collision_map, x, LEVEL3_RESET_ROW, COLLISION_CELL_TRIGGER);

    // Flag de Saída
    collision_matrix_set(&common->collision_map, LEVEL3_GOAL_X, LEVEL3_GOAL_Y, 3);
}

static bool level3_setup_bindings(LevelState* common, Level3State* local, Entity* wall, Entity* harm)
{
    if (common == NULL || local == NULL || wall == NULL || harm == NULL)
        return false;

    wall->user_data = local;
    harm->user_data = local;

    // Linha 0, Token 4 controla a constante numérica 'n'
    if (!codebinding_registry_add(&common->binding_registry, wall, 0, 4, level3_wall_on_token))
        return false;

    // Linha 5, Token 2 controla o operador '>'
    return codebinding_registry_add(&common->binding_registry, harm, 5, 2, level3_harm_on_token);
}

static CodeFile* create_level3_codefile(void)
{
    CodeFile* file = create_codefile();
    set_codefile_terminal_prompt(file, "codeworld@level3:/sys/kernel$ edit sys_init.cpp");
    set_codefile_compile_command(file, "g++ sys_init.cpp -O2 -o sys_init && ./sys_init");

    // Linha 0: const int n = 10;
    CodeLine* line = push_back_empty_codeline(file);
    push_immutable_token(line, "const", criar_cor(220, 200, 90));
    push_immutable_token(line, "int", criar_cor(90, 220, 110));
    push_immutable_token(line, "n", criar_cor(90, 150, 230));
    push_immutable_token(line, "=", criar_cor(220, 90, 90));
    push_immutable_token(line, "10", criar_cor(180, 120, 220));
        line->tokens[line->size - 1].type = TOKEN_NUMBER;
        line->tokens[line->size - 1].slot_type = TOKEN_NUMBER;
    push_immutable_token(line, ";", criar_cor(220, 200, 90));

    // Linha 1: int size = min(max(0, n), 12);
    line = push_back_empty_codeline(file);
    push_immutable_token(line, "int", criar_cor(90, 220, 110));
    push_immutable_token(line, "size", criar_cor(90, 150, 230));
    push_immutable_token(line, "=", criar_cor(220, 90, 90));
    push_immutable_token(line, "min(max(0,", criar_cor(220, 200, 90));
    push_immutable_token(line, "n),", criar_cor(90, 150, 230));
    push_immutable_token(line, "12);", criar_cor(180, 120, 220));

    // Linha 2: Wall* w = (Wall*)malloc(size * sizeof(Wall));
    line = push_back_empty_codeline(file);
    push_immutable_token(line, "Wall*", criar_cor(90, 220, 110));
    push_immutable_token(line, "w", criar_cor(90, 150, 230));
    push_immutable_token(line, "=", criar_cor(220, 90, 90));
    push_immutable_token(line, "(Wall*)malloc(size", criar_cor(220, 200, 90));
    push_immutable_token(line, "*", criar_cor(220, 90, 90));
    push_immutable_token(line, "sizeof(Wall));", criar_cor(220, 200, 90));

    line = push_back_empty_codeline(file);
    push_immutable_token(line, "for", criar_cor(90, 220, 110));
    push_immutable_token(line, "(", criar_cor(220, 90, 90));
    push_immutable_token(line, "int", criar_cor(90, 220, 110));
    push_immutable_token(line, "i", criar_cor(90, 150, 230));
    push_immutable_token(line, "=", criar_cor(220, 90, 90));
    push_immutable_token(line, "0;", criar_cor(180, 120, 220));
    push_immutable_token(line, "i", criar_cor(90, 150, 230));
    push_immutable_token(line, "<", criar_cor(220, 90, 90));
    push_immutable_token(line, "size;", criar_cor(180, 120, 220));
    push_immutable_token(line, "i++;", criar_cor(90, 150, 230));
    push_immutable_token(line, ")", criar_cor(220, 90, 90));

    line = push_back_empty_codeline(file);
    push_indent_token(line, 4);
    push_immutable_token(line, "build_wall_tile(", criar_cor(90, 150, 230));
    push_immutable_token(line, "w[i]", criar_cor(90, 150, 230));
    push_immutable_token(line, ");", criar_cor(90, 150, 230));

    // Linha 5: if (player->vida > 0) {
    line = push_back_empty_codeline(file);
    push_immutable_token(line, "if", criar_cor(90, 220, 110));
    push_immutable_token(line, "(player->vida", criar_cor(220, 200, 90));
    push_immutable_token(line, ">", criar_cor(220, 90, 90));
        line->tokens[line->size - 1].type = TOKEN_OPERATOR;
        line->tokens[line->size - 1].slot_type = TOKEN_OPERATOR;
    push_immutable_token(line, "0)", criar_cor(220, 200, 90));
    push_immutable_token(line, "{", criar_cor(220, 90, 90));

    // Linha 6:     build_harm_block();
    line = push_back_empty_codeline(file);
    push_indent_token(line, 4);
    push_immutable_token(line, "build_harm_block();", criar_cor(90, 150, 230));

    // Linha 7: }
    line = push_back_empty_codeline(file);
    push_immutable_token(line, "}", criar_cor(220, 90, 90));

    return file;
}

static bool level3_init(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL || ctx == NULL || ctx->game == NULL)
        return false;

    LevelState* common = &level->common;
    Level3State* local = (Level3State*)malloc(sizeof(Level3State));
    if (local == NULL)
        return false;

    level->state = local;
    local->common = common;
    local->floor_platform_index = -1;
    local->wall_index = -1;
    local->harm_block_index = -1;
    local->exit_portal_index = -1;
    local->level_label = NULL;
    local->pending_compile_transition = false;
    
    // Configurações lógicas iniciais do código
    local->n_value = 10;
    local->harm_active = true;

    level_state_init(common, ctx->game->curViewMode);
    level3_setup_collision_map(common);

    common->player = create_player(new_Vector2(-44, -4));
    if (common->player == NULL)
    {
        free(local);
        level->state = NULL;
        return false;
    }

    common->player->user_data = common;
    common->player->on_triggering = level3_player_on_triggering;
    common->player_spawn = common->player->position;

    codebinding_registry_init(&common->binding_registry);
    codeeditor_init(&common->editor_session, create_level3_codefile());
    codeeditor_set_bindings(&common->editor_session, &common->binding_registry);

    // Instanciar Entidades Físicas Estáticas e Dinâmicas
    Entity* floor_platform = create_entity(
        level3_cell_center_world(common, LEVEL3_FLOOR_START, LEVEL3_FLOOR_END, LEVEL3_PLATFORM_Y),
        &((Vector2){LEVEL3_FLOOR_END - LEVEL3_FLOOR_START + 1, 2}),
        NULL
    );
    Entity* wall = create_entity(VETOR_NULO, &((Vector2){3, 10}), NULL);
    Entity* harm_block = create_entity(
        level3_cell_center_world(common, LEVEL3_HARM_X_START, LEVEL3_HARM_X_END, LEVEL3_PLATFORM_Y - 1),
        &((Vector2){LEVEL3_HARM_X_END - LEVEL3_HARM_X_START + 1, 1}),
        NULL
    );
    Entity* exit_portal = create_entity(
        level3_cell_world(common, LEVEL3_GOAL_X, LEVEL3_GOAL_Y),
        &((Vector2){5, 8}),
        NULL
    );

    if (floor_platform != NULL) floor_platform->position.y += LEVEL3_PLATFORM_VISUAL_OFFSET_Y;
    if (exit_portal != NULL)    exit_portal->position.y += LEVEL3_GOAL_VISUAL_OFFSET_Y;

    if (floor_platform == NULL || wall == NULL || harm_block == NULL || exit_portal == NULL)
    {
        destroy_entity(floor_platform);
        destroy_entity(wall);
        destroy_entity(harm_block);
        destroy_entity(exit_portal);
        level_state_destroy(common);
        free(local);
        level->state = NULL;
        return false;
    }

    entity_attach_representation(floor_platform, level3_create_platform_visual(criar_cor(40, 45, 60), nv2(floor_platform->size.x, floor_platform->size.y)), true);
    entity_attach_representation(wall,           level3_create_platform_visual(criar_cor(180, 70, 70), nv2(wall->size.x, wall->size.y)), true);
    entity_attach_representation(harm_block,     level3_create_platform_visual(criar_cor(230, 40, 40), nv2(harm_block->size.x, harm_block->size.y)), true);
    entity_attach_representation(exit_portal,    level3_create_platform_visual(criar_cor(80, 220, 90), nv2(exit_portal->size.x, exit_portal->size.y)), true);

    local->level_label = criar_objeto_de_texto(1, 1, "Level 3");
    if (local->level_label != NULL)
    {
        centralizar_objeto(local->level_label);
        local->level_label->position = new_Vector2(-49, -13);
    }

    entity_set_visible(floor_platform, true);
    entity_set_visible(exit_portal, true);

    floor_platform->collidable = false;
    wall->collidable = true;
    harm_block->collidable = false;
    exit_portal->collidable = false;

    if (!level_state_add_entity(common, floor_platform, &local->floor_platform_index)
        || !level_state_add_entity(common, wall,           &local->wall_index)
        || !level_state_add_entity(common, harm_block,     &local->harm_block_index)
        || !level_state_add_entity(common, exit_portal,    &local->exit_portal_index))
    {
        destroy_entity(floor_platform);
        destroy_entity(wall);
        destroy_entity(harm_block);
        destroy_entity(exit_portal);
        if (local->level_label != NULL) excluir_objeto(local->level_label);
        level_state_destroy(common);
        free(local);
        level->state = NULL;
        return false;
    }

    if (!level3_setup_bindings(common, local, wall, harm_block))
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
    level3_sync_world_state(common, local);

    return true;
}

static void level3_handle_input(LevelInstance* level, LevelContext* ctx, char input)
{
    if (level == NULL || ctx == NULL)
        return;

    LevelState* common = &level->common;
    Level3State* local = (Level3State*)level->state;

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
    }
    else if (input == 'D')
    {
        move_code_cursor(&common->cursor, common->editor_session.editor_codefile, 1, 0);
        codeeditor_mark_terminal_cursor_dirty(&common->editor_session);
    }
    else if (input == 'W')
    {
        move_code_cursor(&common->cursor, common->editor_session.editor_codefile, 0, -1);
        codeeditor_mark_terminal_cursor_dirty(&common->editor_session);
    }
    else if (input == 'S')
    {
        move_code_cursor(&common->cursor, common->editor_session.editor_codefile, 0, 1);
        codeeditor_mark_terminal_cursor_dirty(&common->editor_session);
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
                    goto level3_terminal_input_done;
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

level3_terminal_input_done:
    if (terminal_input_changed)
        codeeditor_mark_terminal_dirty(&common->editor_session);
}

static void level3_tick(LevelInstance* level, LevelContext* ctx)
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

        bool supported_before = physics_body_has_support(&common->collision_map, player->position, player->sprite->size);

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
            level3_player_trigger_proxy
        );

        player->position = result.position;
        player->sprite->position = player->position;

        if (result.blocked_x)
            player->velocity.x = 0;

        bool supported_after = physics_body_has_support(&common->collision_map, player->position, player->sprite->size);

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
        level3_reset_player(common->player);
        return;
    }

    if (level_state_player_overlaps_cell_type(common, 3))
        level_request_exit(level, LEVEL_EXIT_COMPLETED, 1);
}

static void level3_draw(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL || ctx == NULL)
        return;

    LevelState* common = &level->common;
    Level3State* local = (Level3State*)level->state;

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

    // Camada de Depuração de Memória Dinâmica (Exibe o Dump de alocação da Parede)
    if (ctx->game->curViewMode == DEBUG_MEMORY && local != NULL)
    {
        level3_draw_entity_if_present(common, ctx->game, local->floor_platform_index);
        level3_draw_entity_if_present(common, ctx->game, local->exit_portal_index);
        player_draw_view(common->player, ctx->game);
        ctx->game->render(ctx->game);

        print_rgb_txt_bg(criar_cor(255, 255, 255), criar_cor(15, 20, 30), new_Vector2(3, 4),  "[HEAP MEMORY DUMP - SECTION: WALL_ALLOC]");
        print_rgb_txt_bg(criar_cor(200, 220, 255), criar_cor(15, 20, 30), new_Vector2(3, 6),  "Address: 0x00A0 [CHUNK START] - Allocated: %d / 12 items", local->n_value);
        print_rgb_txt_bg(criar_cor(160, 160, 160), criar_cor(15, 20, 30), new_Vector2(3, 8),  "Memory layout is bound contiguously to active Wall instances.");
        
        esperar(120);
        player_hide_view(common->player, ctx->game);
        level_state_hide_entities(common, ctx->game);
        return;
    }

    // Camada de Depuração de Estado (Exibe as condições booleanas em Runtime da faixa de teleporte)
    if (ctx->game->curViewMode == DEBUG_STATE && local != NULL)
    {
        level3_draw_entity_if_present(common, ctx->game, local->floor_platform_index);
        level3_draw_entity_if_present(common, ctx->game, local->exit_portal_index);
        player_draw_view(common->player, ctx->game);
        ctx->game->render(ctx->game);

        print_rgb_txt_bg(criar_cor(255, 255, 255), criar_cor(15, 20, 30), new_Vector2(3, 4),  "[RUNTIME STATE MONITOR]");
        print_rgb_txt_bg(criar_cor(255, 200, 200), criar_cor(15, 20, 30), new_Vector2(3, 6),  "player->vida = 100;");
        print_rgb_txt_bg(criar_cor(200, 255, 200), criar_cor(15, 20, 30), new_Vector2(3, 8),  "Condition Checked: (100 %s 0) -> %s", local->harm_active ? ">" : "<", local->harm_active ? "TRUE" : "FALSE");
        print_rgb_txt_bg(criar_cor(255, 255, 100), criar_cor(15, 20, 30), new_Vector2(3, 10), "Harm Block Instance: %s", local->harm_active ? "ACTIVE & ARMED" : "DISABLED");

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

static void level3_destroy(LevelInstance* level, LevelContext* ctx)
{
    if (level == NULL)
        return;

    Level3State* local = (Level3State*)level->state;
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

static const LevelDefinition LEVEL_3_DEFINITION = {
    .id = "level_3",
    .display_name = "Level 3 - Allocation & Operators",
    .world_size = {SCREEN_SIZE_X, SCREEN_SIZE_Y},
    .allow_camera_move = false,
    .vtable = {
        .init = level3_init,
        .handle_input = level3_handle_input,
        .tick = level3_tick,
        .draw = level3_draw,
        .destroy = level3_destroy
    }
};

static const LevelDefinition* level3_get_definition(void)
{
    return &LEVEL_3_DEFINITION;
}

#endif