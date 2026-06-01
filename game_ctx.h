#ifndef GAME_CTX
#define GAME_CTX

#define USE_SHORTCUTS
#include "include/graphycs_all.h"

#define SCREEN_SIZE_X 125
#define SCREEN_SIZE_Y 33
#define QTD_DEBUG_TYPES 5

typedef enum ViewMode {
    VISUAL, DEBUG_COLLISION, DEBUG_MEMORY, DEBUG_STATE, TERMINAL
} ViewMode;

typedef enum GameMode {
    GAMELOOP, CODE
} GameMode;

typedef struct GameContext {
    bool codeModeActive;
    ViewMode curViewMode;
    Screen* screens[QTD_DEBUG_TYPES];
    Objeto* terminalBorder;
    Screen* (*curScreen)(struct GameContext* ctx);
    void (*render)(struct GameContext* ctx);
} GameContext;

Screen* __curScreen(GameContext* ctx) {return ctx->screens[ctx->curViewMode]; }
void __renderCurrScreen(GameContext* ctx) { render(ctx->curScreen(ctx), true); }

GameContext* create_game_ctx()
{
    GameContext* ctx = (GameContext*)malloc(sizeof(GameContext));
    for (int i = 0; i < QTD_DEBUG_TYPES; i++)
        ctx->screens[i] = criar_tela(
            nv2(SCREEN_SIZE_X, SCREEN_SIZE_Y), criar_cor(200, 200, 200), 20
        );

    ctx->terminalBorder = criar_retangulo_monocromatico(COLOR_PRETO, nv2(SCREEN_SIZE_X - 4, SCREEN_SIZE_Y - 4));
    centralizar_objeto(ctx->terminalBorder);
    desenhar_objeto(ctx->screens[TERMINAL], ctx->terminalBorder);
    
    ctx->codeModeActive = false;
    ctx->curViewMode = VISUAL;
    ctx->curScreen = __curScreen;
    ctx->render = __renderCurrScreen;
    return ctx;
}

void freeGameCtx(GameContext* ctx)
{
    if (ctx->terminalBorder != NULL)
        excluir_objeto(ctx->terminalBorder);
    for (int i = 0; i < QTD_DEBUG_TYPES; i++) excluir_tela(ctx->screens[i]);
    free(ctx);
}

#endif