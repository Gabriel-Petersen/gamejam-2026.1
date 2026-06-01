#ifndef PLAYER_H
#define PLAYER_H

#include "game_ctx.h"
#include "player_asset.h"

typedef struct Player Player;
typedef void (*PlayerTriggerFn)(Player* player, uint8_t cell_type, Vector2 cell_pos, GameContext* ctx);

typedef enum PlayerState {
    PLAYER_STATE_IDLE,
    PLAYER_STATE_WALK,
    PLAYER_STATE_HIT,
    PLAYER_STATE_DIE,
    PLAYER_STATE_JUMP,
    PLAYER_STATE_COUNT
} PlayerState;

typedef struct Player {
    ObjetoComplexo* sprite;
    Vector2 position;
    Vector2 velocity;
    PlayerState state;
    bool grounded;
    bool alive;
    int facing;
    const char* name;
    void* user_data;
    PlayerTriggerFn on_triggering;
} Player;

static const char* PLAYER_STATE_NAMES[PLAYER_STATE_COUNT] = {
    "idle",
    "walk",
    "hit",
    "die",
    "jump"
};

static void player_install_animations(Player* player)
{
    static int idle_frames[] = {0, 1};
    static int walk_frames[] = {0, 2};
    static int hit_frames[] = {3};
    static int die_frames[] = {4};
    static int jump_frames[] = {5};

    static Animation animations[PLAYER_STATE_COUNT];
    static bool initialized = false;

    if (!initialized)
    {
        animations[PLAYER_STATE_IDLE] = criar_anim(idle_frames, 2, "idle");
        animations[PLAYER_STATE_WALK] = criar_anim(walk_frames, 2, "walk");
        animations[PLAYER_STATE_HIT] = criar_anim(hit_frames, 1, "hit");
        animations[PLAYER_STATE_DIE] = criar_anim(die_frames, 1, "die");
        animations[PLAYER_STATE_JUMP] = criar_anim(jump_frames, 1, "jump");
        initialized = true;
    }

    setup_animations(player->sprite, animations, PLAYER_STATE_COUNT);
    player->sprite->animar = true;
}

void player_apply_state(Player* player, PlayerState state)
{
    if (player == NULL || player->sprite == NULL)
        return;

    if (player->state == state && player->sprite->anim_manager != NULL)
        return;

    player->state = state;
    setar_animation_via_index(player->sprite, (int)state);
    player->sprite->anim_manager->anims[(int)state].frame_atual = -1;
    player->sprite->frame_atual = player->sprite->anim_manager->anims[(int)state].frame_index[0];
}

static Player* create_player(Vector2 start_pos)
{
    Player* player = (Player*)malloc(sizeof(Player));
    player->sprite = criar_objeto_complexo_piskel(
        PLAYER_FRAME_COUNT,
        PLAYER_FRAME_WIDTH,
        PLAYER_FRAME_HEIGHT,
        player_data
    );
    centralizar_objeto_complexo(player->sprite);
    player->position = start_pos;
    player->velocity = VETOR_NULO;
    player->state = PLAYER_STATE_IDLE;
    player->grounded = true;
    player->alive = true;
    player->facing = 1;
    player->name = "player";
    player->user_data = NULL;
    player->on_triggering = NULL;

    player_install_animations(player);
    player_apply_state(player, PLAYER_STATE_IDLE);
    player->sprite->position = player->position;
    return player;
}

static void destroy_player(Player* player)
{
    if (player == NULL)
        return;

    excluir_objeto_complexo(player->sprite);
    free(player);
}

static void player_step_animation(Player* player)
{
    if (player == NULL || player->sprite == NULL || player->sprite->anim_manager == NULL)
        return;

    Animation* current_anim = &player->sprite->anim_manager->anims[(int)player->state];
    if (current_anim->qtd_frames <= 0 || current_anim->frame_index == NULL)
        return;

    current_anim->frame_atual = (current_anim->frame_atual + 1) % current_anim->qtd_frames;
    player->sprite->frame_atual = current_anim->frame_index[current_anim->frame_atual];
}

static void player_update(Player* player, GameContext* ctx, char input)
{
    if (player == NULL || ctx == NULL)
        return;

    if (!player->alive)
    {
        player_apply_state(player, PLAYER_STATE_DIE);
        player_step_animation(player);
        return;
    }

    if (input == 'K')
    {
        player->alive = false;
        player->grounded = false;
        player->velocity = VETOR_NULO;
        player_apply_state(player, PLAYER_STATE_DIE);
        player_step_animation(player);
        return;
    }

    if (input == 'H')
    {
        player_apply_state(player, PLAYER_STATE_HIT);
    }
    else if ((input == 'W' || input == ' ') && player->grounded)
    {
        player->grounded = false;
        player->velocity.y = -4;
        player_apply_state(player, PLAYER_STATE_JUMP);
    }
    else if (input == 'A')
    {
        player->velocity.x = -2;
        if (player->facing != -1)
        {
            espelhar_objeto_complexo(ctx->curScreen(ctx), player->sprite, true);
            player->facing = -1;
        }
        player_apply_state(player, PLAYER_STATE_WALK);
    }
    else if (input == 'D')
    {
        player->velocity.x = 2;
        if (player->facing != 1)
        {
            espelhar_objeto_complexo(ctx->curScreen(ctx), player->sprite, true);
            player->facing = 1;
        }
        player_apply_state(player, PLAYER_STATE_WALK);
    }
    else if (player->grounded)
    {
        player->velocity.x = 0;
        player_apply_state(player, PLAYER_STATE_IDLE);
    }
    player_step_animation(player);
}

static void player_draw_view(Player* player, GameContext* ctx)
{
    if (player == NULL || ctx == NULL)
        return;

    if (ctx->curViewMode != TERMINAL)
    {
#ifndef DEBUG_ENABLE
        DEBUG_LOG("[PL] draw enter player=%p sprite=%p mode=%d frame=%d qframes=%d renderizado=%d\n",
            (void*)player,
            (void*)player->sprite,
            (int)ctx->curViewMode,
            player->sprite != NULL ? player->sprite->frame_atual : -1,
            player->sprite != NULL ? player->sprite->qtd_frames : -1,
            player->sprite != NULL ? player->sprite->renderizado : -1);
#endif
    }

    if (ctx->curViewMode == TERMINAL)
    {
        print_rgb_txt(
            NULL,
            criar_cor(220, 220, 220),
            new_Vector2(2, 2),
            "PLAYER [%s] state=%s pos=(%d,%d)\n",
            player->name,
            PLAYER_STATE_NAMES[(int)player->state],
            player->position.x,
            player->position.y
        );
        return;
    }

    if (player->sprite == NULL || player->sprite->qtd_frames <= 0)
    {
    #ifndef DEBUG_ENABLE
        DEBUG_LOG("[PL] draw abort: invalid sprite or frame count\n");
    #endif
        return;
    }

    if (player->sprite->frame_atual < 0 || player->sprite->frame_atual >= player->sprite->qtd_frames)
    {
#ifndef DEBUG_ENABLE
        DEBUG_LOG("[PL] draw clamp frame from %d to 0\n", player->sprite->frame_atual);
#endif
        player->sprite->frame_atual = 0;
    }

    player->sprite->position = player->position;
#ifndef DEBUG_ENABLE
    if (ctx->curViewMode != TERMINAL)
        DEBUG_LOG("[PL] draw before-hide-check renderizado=%d\n", player->sprite->renderizado);
#endif
    if (player->sprite->renderizado)
    {
#ifndef DEBUG_ENABLE
        DEBUG_LOG("[PL] draw hide old sprite\n");
#endif
        esconder_objeto_complexo(ctx->curScreen(ctx), player->sprite);
    }

#ifndef DEBUG_ENABLE
    DEBUG_LOG("[PL] draw before-draw\n");
#endif
    desenhar_objeto_complexo(ctx->curScreen(ctx), player->sprite);
#ifndef DEBUG_ENABLE
    DEBUG_LOG("[PL] draw after-draw\n");
#endif
}

static void player_hide_view(Player* player, GameContext* ctx)
{
    if (player == NULL || ctx == NULL || ctx->curViewMode == TERMINAL)
        return;

    if (player->sprite->renderizado)
        esconder_objeto_complexo(ctx->curScreen(ctx), player->sprite);
}

#endif