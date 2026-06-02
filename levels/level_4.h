#ifndef LEVEL_4_H
#define LEVEL_4_H

#include <stdlib.h>
#include <string.h>
#include "../level.h"

// --- DIMENSÕES DO EXAME FINAL ---
enum {
    L4_PLATFORM_Y = 24,
    L4_VISUAL_OFFSET_Y = 1,
    L4_GOAL_OFFSET_Y = -4,
    
    L4_LEFT_START = 5,   L4_LEFT_END = 25,
    L4_BRIDGE_START = 26, L4_BRIDGE_END = 45,
    L4_MID_START = 46,   L4_MID_END = 70,
    L4_WALL_START = 55,  L4_WALL_END = 57,
    L4_HARM_START = 71,  L4_HARM_END = 90,
    L4_RIGHT_START = 91, L4_RIGHT_END = 115,
    
    L4_GOAL_X = 110,
    L4_GOAL_Y = 23,
    L4_RESET_ROW = 31
};

typedef struct Level4State {
    LevelState* common;
    int left_idx, mid_idx, right_idx;
    int bridge_idx, wall_idx, harm_idx, goal_idx;
    Objeto* level_label;
    bool pending_compile_transition;
    
    // Variáveis de Runtime (The Final Exam)
    bool bridge_open;
    int wall_height;
    bool harm_active;
} Level4State;

// --- UTILS ---
static Vector2 l4_cell(LevelState* c, int x, int y) { return collision_matrix_cell_world(&c->collision_map, x, y); }
static Vector2 l4_center(LevelState* c, int sx, int ex, int y) {
    Vector2 l = l4_cell(c, sx, y); return new_Vector2((l.x + l4_cell(c, ex, y).x)/2, l.y);
}
static void l4_fill(CollisionMatrix* m, int sx, int ex, int y, uint8_t val) {
    for(int x=sx; x<=ex; x++) collision_matrix_set(m, x, y, val);
}

// --- PLAYER TRIGGERS ---
static void l4_reset_player(Player* p) {
    if(!p) return;
    LevelState* c = (LevelState*)p->user_data;
    level_state_reset_player(c);
    p->velocity = VETOR_NULO;
    p->grounded = true;
    player_apply_state(p, PLAYER_STATE_IDLE);
}

static void l4_player_trigger(Player* p, uint8_t type, Vector2 pos, GameContext* ctx) {
    if(p && type == COLLISION_CELL_TRIGGER) l4_reset_player(p);
    (void)pos; (void)ctx;
}

static void l4_trigger_proxy(void* a, uint8_t t, Vector2 p, GameContext* ctx) {
    Player* pl = (Player*)a; if(pl && pl->on_triggering) pl->on_triggering(pl, t, p, ctx);
}

static ObjetoComplexo* l4_visual(Color c, Vector2 s) {
    Objeto** f = (Objeto**)malloc(sizeof(Objeto*));
    f[0] = criar_retangulo_monocromatico(c, s);
    f[0]->position = VETOR_NULO;
    ObjetoComplexo* rep = criar_objeto_complexo_via_lista(f, 1);
    centralizar_objeto_complexo(rep);
    return rep;
}

// --- CORE SYNC ---
static void l4_sync_world(LevelState* common, Level4State* local) {
    if(!common || !local) return;

    // 1. Bridge Sync
    l4_fill(&common->collision_map, L4_BRIDGE_START, L4_BRIDGE_END, L4_PLATFORM_Y, local->bridge_open ? COLLISION_CELL_SOLID : COLLISION_CELL_FREE);
    Entity* b = level_state_entity_at(common, local->bridge_idx);
    if(b) { entity_set_visible(b, local->bridge_open); b->collidable = local->bridge_open; }

    // 2. Wall Sync
    for(int x=L4_WALL_START; x<=L4_WALL_END; x++)
        for(int y=L4_PLATFORM_Y-12; y<L4_PLATFORM_Y; y++)
            collision_matrix_set(&common->collision_map, x, y, COLLISION_CELL_FREE);
            
    int wh = local->wall_height;
    if(wh>12) wh=12; if(wh<0) wh=0;
    
    Entity* w = level_state_entity_at(common, local->wall_idx);
    if(wh>0) {
        for(int x=L4_WALL_START; x<=L4_WALL_END; x++)
            for(int y=L4_PLATFORM_Y-wh; y<L4_PLATFORM_Y; y++)
                collision_matrix_set(&common->collision_map, x, y, COLLISION_CELL_SOLID);
        if(w) {
            Vector2 bot = l4_center(common, L4_WALL_START, L4_WALL_END, L4_PLATFORM_Y-1);
            Vector2 top = l4_center(common, L4_WALL_START, L4_WALL_END, L4_PLATFORM_Y-wh);
            w->position = new_Vector2((bot.x+top.x)/2, (bot.y+top.y)/2);
            w->size = new_Vector2(L4_WALL_END-L4_WALL_START+1, wh);
            if(w->representation) w->representation->position = w->position;
            entity_set_visible(w, true); w->collidable = true;
        }
    } else if(w) {
        entity_set_visible(w, false); w->collidable = false;
    }

    // 3. Harm Pit Sync (Laser grid)
    uint8_t hv = local->harm_active ? COLLISION_CELL_TRIGGER : COLLISION_CELL_FREE;
    l4_fill(&common->collision_map, L4_HARM_START, L4_HARM_END, L4_PLATFORM_Y-1, hv);
    Entity* h = level_state_entity_at(common, local->harm_idx);
    if(h) { 
        entity_set_visible(h, local->harm_active); 
        h->collidable = false;
    }
}

// --- HANDLERS ---
static void l4_bridge_tk(Entity* e, CodeToken* tk, GameContext* c) {
    Level4State* l = (Level4State*)e->user_data;
    if(tk->type != TOKEN_NULL) l->bridge_open = (strcmp(tk->string, "true")==0 || strcmp(tk->string, "1")==0);
    l4_sync_world(l->common, l);
}
static void l4_wall_tk(Entity* e, CodeToken* tk, GameContext* c) {
    Level4State* l = (Level4State*)e->user_data;
    l->wall_height = (tk->type == TOKEN_NULL) ? 0 : atoi(tk->string);
    l4_sync_world(l->common, l);
}
static void l4_harm_tk(Entity* e, CodeToken* tk, GameContext* c) {
    Level4State* l = (Level4State*)e->user_data;
    l->harm_active = (tk->type != TOKEN_NULL && strchr(tk->string, '>') != NULL);
    l4_sync_world(l->common, l);
}

// --- FILE GEN ---
static CodeFile* create_level4_codefile(void) {
    CodeFile* f = create_codefile();
    set_codefile_terminal_prompt(f, "root@core:/sys$ edit final_override.cpp");
    set_codefile_compile_command(f, "make final_override && ./final_override");

    CodeLine* l;

    // Linha 0: bool bridge_on = false ;
    l = push_back_empty_codeline(f);
    push_immutable_token(l, "bool", criar_cor(90, 220, 110));
    push_immutable_token(l, "bridge_on", criar_cor(90, 150, 230));
    push_immutable_token(l, "=", criar_cor(220, 90, 90));
    push_immutable_token(l, "false", criar_cor(180, 120, 220));
        l->tokens[l->size-1].type = TOKEN_BOOL; l->tokens[l->size-1].slot_type = TOKEN_BOOL;
    push_immutable_token(l, ";", criar_cor(220, 200, 90));

    // Linha 1: int wall_h = 10 ;
    l = push_back_empty_codeline(f);
    push_immutable_token(l, "int", criar_cor(90, 220, 110));
    push_immutable_token(l, "wall_h", criar_cor(90, 150, 230));
    push_immutable_token(l, "=", criar_cor(220, 90, 90));
    push_immutable_token(l, "10", criar_cor(180, 120, 220));
        l->tokens[l->size-1].type = TOKEN_NUMBER; l->tokens[l->size-1].slot_type = TOKEN_NUMBER;
    push_immutable_token(l, ";", criar_cor(220, 200, 90));

    // Linha 2: if (player->vida > 0) {
    l = push_back_empty_codeline(f);
    push_immutable_token(l, "if", criar_cor(90, 220, 110));
    push_immutable_token(l, "(player->vida", criar_cor(220, 200, 90));
    push_immutable_token(l, ">", criar_cor(220, 90, 90));
        l->tokens[l->size-1].type = TOKEN_OPERATOR; l->tokens[l->size-1].slot_type = TOKEN_OPERATOR;
    push_immutable_token(l, "0)", criar_cor(220, 200, 90));
    push_immutable_token(l, "{", criar_cor(220, 90, 90));
    
    // Linha 3: \t enable_laser_pit();
    l = push_back_empty_codeline(f);
    push_indent_token(l, 4);
    push_immutable_token(l, "enable_laser_pit();", criar_cor(90, 150, 230));

    // Linha 4: }
    l = push_back_empty_codeline(f);
    push_immutable_token(l, "}", criar_cor(220, 90, 90));

    return f;
}

static bool level4_init(LevelInstance* lvl, LevelContext* ctx) {
    LevelState* c = &lvl->common;
    Level4State* l = (Level4State*)malloc(sizeof(Level4State));
    lvl->state = l; l->common = c;
    l->bridge_open = false; l->wall_height = 10; l->harm_active = true;
    l->pending_compile_transition = false;
    
    level_state_init(c, ctx->game->curViewMode);
    collision_matrix_init(&c->collision_map, SCREEN_SIZE_X, SCREEN_SIZE_Y, new_Vector2(-(SCREEN_SIZE_X/2), -(SCREEN_SIZE_Y/2)));
    collision_matrix_fill(&c->collision_map, COLLISION_CELL_FREE);

    l4_fill(&c->collision_map, L4_LEFT_START, L4_LEFT_END, L4_PLATFORM_Y, COLLISION_CELL_SOLID);
    l4_fill(&c->collision_map, L4_MID_START, L4_MID_END, L4_PLATFORM_Y, COLLISION_CELL_SOLID);
    l4_fill(&c->collision_map, L4_RIGHT_START, L4_RIGHT_END, L4_PLATFORM_Y, COLLISION_CELL_SOLID);
    for(int x=0; x<c->collision_map.width; x++) collision_matrix_set(&c->collision_map, x, L4_RESET_ROW, COLLISION_CELL_TRIGGER);
    collision_matrix_set(&c->collision_map, L4_GOAL_X, L4_GOAL_Y, 3);

    c->player = create_player(new_Vector2(-48, -4));
    c->player->user_data = c; c->player->on_triggering = l4_player_trigger; c->player_spawn = c->player->position;

    codebinding_registry_init(&c->binding_registry);
    codeeditor_init(&c->editor_session, create_level4_codefile());
    codeeditor_set_bindings(&c->editor_session, &c->binding_registry);

    Entity* e_l = create_entity(l4_center(c, L4_LEFT_START, L4_LEFT_END, L4_PLATFORM_Y), &((Vector2){L4_LEFT_END-L4_LEFT_START+1,2}), NULL);
    Entity* e_m = create_entity(l4_center(c, L4_MID_START, L4_MID_END, L4_PLATFORM_Y), &((Vector2){L4_MID_END-L4_MID_START+1,2}), NULL);
    Entity* e_r = create_entity(l4_center(c, L4_RIGHT_START, L4_RIGHT_END, L4_PLATFORM_Y), &((Vector2){L4_RIGHT_END-L4_RIGHT_START+1,2}), NULL);
    
    Entity* b = create_entity(l4_center(c, L4_BRIDGE_START, L4_BRIDGE_END, L4_PLATFORM_Y), &((Vector2){L4_BRIDGE_END-L4_BRIDGE_START+1,2}), NULL);
    Entity* w = create_entity(VETOR_NULO, &((Vector2){3,10}), NULL);
    Entity* h = create_entity(l4_center(c, L4_HARM_START, L4_HARM_END, L4_PLATFORM_Y-1), &((Vector2){L4_HARM_END-L4_HARM_START+1,1}), NULL);
    Entity* g = create_entity(l4_cell(c, L4_GOAL_X, L4_GOAL_Y), &((Vector2){5,8}), NULL);

    e_l->position.y += L4_VISUAL_OFFSET_Y; e_m->position.y += L4_VISUAL_OFFSET_Y; e_r->position.y += L4_VISUAL_OFFSET_Y;
    b->position.y += L4_VISUAL_OFFSET_Y; g->position.y += L4_GOAL_OFFSET_Y;

    entity_attach_representation(e_l, l4_visual(criar_cor(40, 45, 60), nv2(e_l->size.x,2)), true);
    entity_attach_representation(e_m, l4_visual(criar_cor(40, 45, 60), nv2(e_m->size.x,2)), true);
    entity_attach_representation(e_r, l4_visual(criar_cor(40, 45, 60), nv2(e_r->size.x,2)), true);
    entity_attach_representation(b, l4_visual(criar_cor(220, 200, 50), nv2(b->size.x,2)), true);
    entity_attach_representation(w, l4_visual(criar_cor(180, 70, 70), nv2(3,10)), true);
    entity_attach_representation(h, l4_visual(criar_cor(230, 40, 40), nv2(h->size.x,1)), true);
    entity_attach_representation(g, l4_visual(criar_cor(80, 220, 90), nv2(5,8)), true);

    e_l->collidable = e_m->collidable = e_r->collidable = g->collidable = false;
    entity_set_visible(e_l, true); entity_set_visible(e_m, true); entity_set_visible(e_r, true); entity_set_visible(g, true);

    level_state_add_entity(c, e_l, &l->left_idx); level_state_add_entity(c, e_m, &l->mid_idx); level_state_add_entity(c, e_r, &l->right_idx);
    level_state_add_entity(c, b, &l->bridge_idx); level_state_add_entity(c, w, &l->wall_idx); level_state_add_entity(c, h, &l->harm_idx);
    level_state_add_entity(c, g, &l->goal_idx);

    b->user_data = w->user_data = h->user_data = l;
    
    // CORREÇÃO DOS ÍNDICES DO BINDING
    codebinding_registry_add(&c->binding_registry, b, 0, 3, l4_bridge_tk); // Linha 0, bool (idx 3)
    codebinding_registry_add(&c->binding_registry, w, 1, 3, l4_wall_tk);   // Linha 1, int (idx 3)
    codebinding_registry_add(&c->binding_registry, h, 2, 2, l4_harm_tk);   // Linha 2, op (idx 2)

    l->level_label = criar_objeto_de_texto(1, 1, "Level 4");
    if(l->level_label) { centralizar_objeto(l->level_label); l->level_label->position = new_Vector2(-49, -13); }

    c->cursor = (CodeCursor){.bg_color = criar_cor(25, 240, 255), .imm_bg_color = criar_cor(255, 235, 40)};
    c->chrome = (CodeEditorChrome){.code_origin = nv2(2, 3), .prompt_origin = nv2(2, 1), .error_origin = nv2(2, 25), .footer_origin = nv2(2, 29)};
    
    l4_sync_world(c, l);
    return true;
}

// --- BOILERPLATE PADRÃO ---
static void level4_handle_input(LevelInstance* lvl, LevelContext* ctx, char in) {
    LevelState* c = &lvl->common; Level4State* l = (Level4State*)lvl->state;
    if(in>='1' && in<='5') ctx->game->curViewMode = (ViewMode)(in-'1');
    if(ctx->game->curViewMode != c->last_view_mode) {
        limpar_buffer(ctx->game->curScreen(ctx->game));
        if(ctx->game->curViewMode == TERMINAL || c->last_view_mode == TERMINAL) {
            codeeditor_prepare_terminal(&c->editor_session); codeeditor_reset_editor(&c->editor_session);
        }
        c->last_view_mode = ctx->game->curViewMode;
    }
    if(ctx->game->curViewMode != TERMINAL) return;
    
    bool dirty = false;
    if(in=='T') { codeeditor_handle_text_submission_shortcut(&c->editor_session, c->cursor, in); dirty=true; }
    else if(in=='A') move_code_cursor(&c->cursor, c->editor_session.editor_codefile, -1, 0);
    else if(in=='D') move_code_cursor(&c->cursor, c->editor_session.editor_codefile, 1, 0);
    else if(in=='W') move_code_cursor(&c->cursor, c->editor_session.editor_codefile, 0, -1);
    else if(in=='S') move_code_cursor(&c->cursor, c->editor_session.editor_codefile, 0, 1);
    else if(in==BACKSPACE_KEY) {
        CodeLine* cl = c->editor_session.editor_codefile->lines[c->cursor.position.y];
        if(cl && cl->size > 0 && c->cursor.position.x < cl->size) {
            CodeToken* tk = &cl->tokens[c->cursor.position.x];
            if(tk->slot_type != TOKEN_IMMUTABLE && tk->type != TOKEN_INDENT) {
                void* tgt = tk->target_ptr; set_token_literal(tk, "_", TOKEN_NULL, criar_cor(160, 160, 160));
                tk->target_ptr = tgt; dirty=true;
            }
        }
    } else if(in=='R') {
        dirty=true;
        if(codeeditor_try_compile_and_run(&c->editor_session, ctx->game)) {
            l->pending_compile_transition = true; ctx->game->curViewMode = TERMINAL;
        } else ctx->game->curViewMode = TERMINAL;
    }
    if(dirty) codeeditor_mark_terminal_dirty(&c->editor_session); else codeeditor_mark_terminal_cursor_dirty(&c->editor_session);
}

static void level4_tick(LevelInstance* lvl, LevelContext* ctx) {
    if(!lvl || !ctx || ctx->game->curViewMode == TERMINAL) return;
    LevelState* c = &lvl->common; Player* p = c->player;
    if(p) player_update(p, ctx->game, ctx->last_input);
    if(p && p->alive) {
        if(!physics_body_has_support(&c->collision_map, p->position, p->sprite->size)) p->grounded = false;
        if(!p->grounded) p->velocity.y += 1;
        PhysicsMoveResult res = physics_resolve_motion(&c->collision_map, p->position, p->sprite->size, p->velocity, p, ctx->game, l4_trigger_proxy);
        p->position = res.position; p->sprite->position = p->position;
        if(res.blocked_x) p->velocity.x = 0;
        if(res.blocked_y || physics_body_has_support(&c->collision_map, p->position, p->sprite->size)) {
            p->grounded = true; p->velocity.y = 0;
        } else if(p->velocity.y != 0) p->grounded = false;
        
        if(p->state != PLAYER_STATE_HIT && p->state != PLAYER_STATE_DIE) {
            if(p->grounded) player_apply_state(p, p->velocity.x ? PLAYER_STATE_WALK : PLAYER_STATE_IDLE);
            else if(p->velocity.y < 0) player_apply_state(p, PLAYER_STATE_JUMP);
        }
    }
    level_state_resolve_player_entity_collisions(c);
    
    // CORREÇÃO DO TRIGGER: O player agora teleporta ao cair no void ou tocar no harm block armado
    if(level_state_player_overlaps_cell_type(c, COLLISION_CELL_TRIGGER)) {
        l4_reset_player(p);
        return;
    }
    
    if(level_state_player_overlaps_cell_type(c, 3)) level_request_exit(lvl, LEVEL_EXIT_COMPLETED, -1);
}

static void level4_draw(LevelInstance* lvl, LevelContext* ctx) {
    LevelState* c = &lvl->common; Level4State* l = (Level4State*)lvl->state;
    if(ctx->game->curViewMode == TERMINAL) {
        codeeditor_render_terminal(&c->editor_session, ctx->game, c->cursor, c->chrome);
        if(l->pending_compile_transition) {
            esperar(1000); ctx->game->curViewMode = VISUAL; limpar_buffer(ctx->game->curScreen(ctx->game));
            clear_codefile_compile_success_message(c->editor_session.editor_codefile);
            l->pending_compile_transition = false; c->last_view_mode = VISUAL;
        }
        return;
    }
    
    if(ctx->game->curViewMode == VISUAL && l->level_label) desenhar_objeto(ctx->game->curScreen(ctx->game), l->level_label);
    
    if(ctx->game->curViewMode == DEBUG_STATE) {
        print_rgb_txt_bg(criar_cor(255, 255, 255), criar_cor(15, 20, 30), new_Vector2(3, 4),  "[THE OVERSEER DUMP]");
        print_rgb_txt_bg(criar_cor(200, 220, 255), criar_cor(15, 20, 30), new_Vector2(3, 6),  "BRIDGE POWER: %s", l->bridge_open ? "ACTIVE" : "OFFLINE");
        print_rgb_txt_bg(criar_cor(255, 200, 200), criar_cor(15, 20, 30), new_Vector2(3, 8),  "WALL ALLOC  : %d CHUNKS", l->wall_height);
        print_rgb_txt_bg(criar_cor(200, 255, 200), criar_cor(15, 20, 30), new_Vector2(3, 10), "LASER GRID  : %s", l->harm_active ? "ARMED" : "SAFE");
    }

    if(c->player) player_draw_view(c->player, ctx->game);
    level_state_draw_entities(c, ctx->game);
    ctx->game->render(ctx->game);
    if(ctx->game->curViewMode == DEBUG_COLLISION) physics_draw_collision_matrix(&c->collision_map, ctx->game->curScreen(ctx->game), c->player->position, c->player->sprite->size, true);

    esperar(120);
    if(c->player) player_hide_view(c->player, ctx->game);
    level_state_hide_entities(c, ctx->game);
}

static void level4_destroy(LevelInstance* lvl, LevelContext* ctx) {
    Level4State* l = (Level4State*)lvl->state;
    if(l && l->level_label) { esconder_objeto(ctx->game->curScreen(ctx->game), l->level_label); excluir_objeto(l->level_label); }
    level_state_hide_renderables(&lvl->common, ctx->game);
    level_state_destroy(&lvl->common); free(lvl->state); lvl->state = NULL;
}

static const LevelDefinition LEVEL_4_DEFINITION = {
    .id = "level_4", .display_name = "Level 4 - The Overseer",
    .world_size = {SCREEN_SIZE_X, SCREEN_SIZE_Y}, .allow_camera_move = false,
    .vtable = { .init = level4_init, .handle_input = level4_handle_input, .tick = level4_tick, .draw = level4_draw, .destroy = level4_destroy }
};

static const LevelDefinition* level4_get_definition(void) { return &LEVEL_4_DEFINITION; }

#endif