#ifndef ENTITY_H
#define ENTITY_H

#include <stdio.h>
#include "game_ctx.h"

typedef struct CodeToken CodeToken;

struct Entity;

typedef int (*EntityFrameSelector)(struct Entity*, GameContext*, ViewMode);
typedef void (*EntityDrawFn)(struct Entity*, GameContext*);
typedef void (*EntityUpdateFn)(struct Entity*, GameContext*);
typedef void (*EntityTokenHandler)(struct Entity*, CodeToken*, GameContext*);

typedef struct Entity {
    int id;
    Vector2 position;
    Vector2 size;
    ObjetoComplexo* representation;
    bool owns_representation;
    bool visible;
    bool collidable;
    void* user_data;
    EntityFrameSelector frame_for_view;
    EntityDrawFn draw;
    EntityDrawFn hide;
    EntityUpdateFn update;
    EntityTokenHandler on_token;
} Entity;

int entity_default_frame_for_view(struct Entity* ent, GameContext* ctx, ViewMode view)
{
    (void)ctx;

    if (ent == NULL || ent->representation == NULL || view == TERMINAL)
        return -1;

    if (ent->representation->qtd_frames <= 0)
        return -1;

    int frame = (int)view;
    if (frame < 0 || frame >= ent->representation->qtd_frames)
        frame = ent->representation->frame_atual;
    if (frame < 0 || frame >= ent->representation->qtd_frames)
        frame = 0;

    return frame;
}

void entity_hide_representation(Entity* ent, GameContext* ctx)
{
    if (ent == NULL || ctx == NULL || ent->representation == NULL)
        return;

    if (ent->representation->renderizado)
        esconder_objeto_complexo(ctx->curScreen(ctx), ent->representation);
}

void entity_draw_representation(Entity* ent, GameContext* ctx)
{
    if (ent == NULL || ctx == NULL || ent->representation == NULL || !ent->visible)
        return;

    ViewMode view = ctx->curViewMode;
    int frame = (ent->frame_for_view != NULL)
        ? ent->frame_for_view(ent, ctx, view)
        : entity_default_frame_for_view(ent, ctx, view);

    if (frame < 0 || frame >= ent->representation->qtd_frames)
    {
        entity_hide_representation(ent, ctx);
        return;
    }

    Screen* screen = ctx->curScreen(ctx);
    if (ent->representation->renderizado)
        esconder_objeto_complexo(screen, ent->representation);

    ent->representation->position = ent->position;
    ent->representation->frame_atual = frame;
    desenhar_objeto_complexo(screen, ent->representation);
}

void entity_draw_current_view(Entity* ent, GameContext* ctx)
{
    if (ent == NULL || ctx == NULL || !ent->visible)
        return;

    if (ent->draw != NULL)
    {
        ent->draw(ent, ctx);
        return;
    }

    entity_draw_representation(ent, ctx);
}

void entity_hide_current_view(Entity* ent, GameContext* ctx)
{
    if (ent == NULL || ctx == NULL)
        return;

    if (ent->hide != NULL)
    {
        ent->hide(ent, ctx);
        return;
    }

    entity_hide_representation(ent, ctx);
}

Entity* create_entity(Vector2 start_pos, Vector2* size, EntityUpdateFn update)
{
    static uint16_t id = 0;
    Entity* ent = (Entity*)malloc(sizeof(Entity));
    ent->id = id++;
    ent->position = start_pos;
    ent->size = (size != NULL) ? *size : VETOR_NULO;
    ent->representation = NULL;
    ent->owns_representation = false;
    ent->visible = false;
    ent->collidable = false;
    ent->user_data = NULL;
    ent->frame_for_view = NULL;
    ent->draw = NULL;
    ent->hide = NULL;
    ent->update = update;
    ent->on_token = NULL;
    return ent;
}

void destroy_entity(Entity* ent)
{
    if (ent == NULL)
        return;

    if (ent->owns_representation && ent->representation != NULL)
        excluir_objeto_complexo(ent->representation);

    free(ent);
}

void entity_attach_representation(Entity* ent, ObjetoComplexo* representation, bool owns_representation)
{
    if (ent == NULL)
        return;

    ent->representation = representation;
    ent->owns_representation = owns_representation;
}

void entity_set_frame_selector(Entity* ent, EntityFrameSelector frame_for_view)
{
    if (ent == NULL)
        return;

    ent->frame_for_view = frame_for_view;
}

void entity_set_draw_fn(Entity* ent, EntityDrawFn draw)
{
    if (ent == NULL)
        return;

    ent->draw = draw;
}

void entity_set_hide_fn(Entity* ent, EntityDrawFn hide)
{
    if (ent == NULL)
        return;

    ent->hide = hide;
}

void entity_set_user_data(Entity* ent, void* user_data)
{
    if (ent == NULL)
        return;

    ent->user_data = user_data;
}

void entity_handle_token(Entity* ent, CodeToken* token, GameContext* ctx)
{
    if (ent == NULL || token == NULL || ent->on_token == NULL)
        return;

    ent->on_token(ent, token, ctx);
}

void entity_set_token_handler(Entity* ent, EntityTokenHandler on_token)
{
    if (ent == NULL)
        return;

    ent->on_token = on_token;
}

void entity_set_visible(Entity* ent, bool visible)
{
    if (ent == NULL)
        return;

    ent->visible = visible;
}

void move_entity(Entity* ent, Vector2 delta)
{
    if (ent == NULL)
        return;

    ent->position = vector_sum(ent->position, delta);
    if (ent->representation != NULL)
        ent->representation->position = ent->position;
}

void entity_update(Entity* ent, GameContext* ctx)
{
    if (ent == NULL)
        return;

    if (ent->update != NULL)
        ent->update(ent, ctx);
}

#endif