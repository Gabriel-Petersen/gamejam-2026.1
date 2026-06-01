#ifndef MENU_H
#define MENU_H

#define USE_SHORTCUTS
#include "game_ctx.h"
#include "include/graphycs_all.h"
#include "player_asset.h"

typedef enum MenuResult {
    MENU_RESULT_PLAY,
    MENU_RESULT_QUIT
} MenuResult;

typedef enum MenuPage {
    MENU_PAGE_MAIN,
    MENU_PAGE_CREDITS,
    MENU_PAGE_TUTORIAL
} MenuPage;

typedef struct MenuState {
    Screen* screen;
    Objeto* title;
    Objeto* selector;
    Objeto* detail_frame;
    Objeto* showcase_player;
    int selected_index;
    MenuPage page;
    bool dirty;
    bool clear_required;
} MenuState;

static Vector2 menu_screen_to_world(Vector2 screen_pos)
{
    return new_Vector2(
        screen_pos.x - (SCREEN_SIZE_X / 2),
        screen_pos.y - (SCREEN_SIZE_Y / 2)
    );
}

static void menu_draw_button(Screen* screen, const char* text, Vector2 pos, bool selected)
{
    Color fg = selected ? criar_cor(255, 235, 110) : criar_cor(220, 220, 220);
    print_rgb_txt(screen, fg, pos, "%s", text);
}

static void menu_draw_overlay(MenuState* state)
{
    if (state == NULL || state->screen == NULL)
        return;

    if (state->page == MENU_PAGE_MAIN)
    {
        menu_draw_button(state->screen, "[ creditos ]", new_Vector2(12, 13), state->selected_index == 0);
        menu_draw_button(state->screen, "[ jogar ]", new_Vector2(13, 17), state->selected_index == 1);
        menu_draw_button(state->screen, "[ tutorial ]", new_Vector2(12, 21), state->selected_index == 2);
        print_rgb_txt(state->screen, criar_cor(180, 180, 180), new_Vector2(8, 26), "W/S navegam | Enter confirma | Q sai");
    }
    else if (state->page == MENU_PAGE_CREDITS)
    {
        print_rgb_txt(state->screen, criar_cor(255, 235, 110), new_Vector2(8, 14), "creditos");
        print_rgb_txt(state->screen, criar_cor(220, 220, 220), new_Vector2(8, 16), "Engine, arte e ideao inicial: prototipo da jam");
        print_rgb_txt(state->screen, criar_cor(220, 220, 220), new_Vector2(8, 18), "UI, bindings e menu: camada de debug do jogo");
        print_rgb_txt(state->screen, criar_cor(180, 180, 180), new_Vector2(8, 22), "Enter/Backspace/Esc volta");
    }
    else if (state->page == MENU_PAGE_TUTORIAL)
    {
        print_rgb_txt(state->screen, criar_cor(255, 235, 110), new_Vector2(8, 14), "tutorial");
        print_rgb_txt(state->screen, criar_cor(220, 220, 220), new_Vector2(8, 16), "W/S troca selecao | Enter abre");
        print_rgb_txt(state->screen, criar_cor(220, 220, 220), new_Vector2(8, 18), "Visual: A/D anda, W ou Espaco pula");
        print_rgb_txt(state->screen, criar_cor(220, 220, 220), new_Vector2(8, 20), "Terminal: Y/N troca token | Backspace apaga");
        print_rgb_txt(state->screen, criar_cor(220, 220, 220), new_Vector2(8, 22), "R compila/aplica quando nao houver TOKEN_NULL");
        print_rgb_txt(state->screen, criar_cor(180, 180, 180), new_Vector2(8, 26), "1-5 troca views | Enter/Backspace/Esc volta");
    }
}

static void menu_apply_selector(MenuState* state)
{
    if (state == NULL || state->screen == NULL || state->selector == NULL)
        return;

    static const Vector2 positions[3] = {
        {8, 12},
        {8, 16},
        {8, 20}
    };

    esconder_objeto(state->screen, state->selector);
    state->selector->position = menu_screen_to_world(positions[state->selected_index]);
    desenhar_objeto(state->screen, state->selector);
}

static void menu_clear_terminal_area(void)
{
    for (int row = 0; row < SCREEN_SIZE_Y; row++)
    {
        moveCursor(new_Vector2(0, row));
        for (int col = 0; col < SCREEN_SIZE_X; col++)
            printf(" ");
    }
}

static MenuResult menu_run(void)
{
    MenuState state = {0};
    state.screen = criar_tela(nv2(SCREEN_SIZE_X, SCREEN_SIZE_Y), COLOR_PRETO, 20);
    state.title = criar_objeto_de_texto(1, 3, "CODEWORLD");
    trocar_cor_texto(state.title, COLOR_BRANCO);
    state.title->position = menu_screen_to_world(new_Vector2(8, 3));

    state.selector = criar_frame_retangular(nv2(22, 3), COLOR_BRANCO);
    state.detail_frame = criar_retangulo_monocromatico(criar_cor(17, 35, 64), nv2(40, 27));
    state.detail_frame->position = menu_screen_to_world(new_Vector2(82, 3));
    state.showcase_player = criar_piskel_obj(player_data[0], PLAYER_FRAME_WIDTH, PLAYER_FRAME_HEIGHT);
    espelhar_objeto(NULL, state.showcase_player, true);
    centralizar_objeto(state.showcase_player);
    state.showcase_player->position = menu_screen_to_world(new_Vector2(101, 15));

    desenhar_objeto(state.screen, state.detail_frame);
    desenhar_objeto(state.screen, state.showcase_player);
    desenhar_objeto(state.screen, state.title);
    state.selected_index = 1;
    state.page = MENU_PAGE_MAIN;
    state.dirty = true;
    state.clear_required = true;
    menu_apply_selector(&state);

    while (true)
    {
        char input = ler_teclado();

        if (input == 'Q')
        {
            excluir_tela(state.screen);
            excluir_objeto(state.title);
            excluir_objeto(state.selector);
            excluir_objeto(state.detail_frame);
            excluir_objeto(state.showcase_player);
            return MENU_RESULT_QUIT;
        }

        if (state.page == MENU_PAGE_MAIN)
        {
            if (input == 'W')
            {
                state.selected_index = (state.selected_index + 2) % 3;
                menu_apply_selector(&state);
                state.dirty = true;
            }
            else if (input == 'S')
            {
                state.selected_index = (state.selected_index + 1) % 3;
                menu_apply_selector(&state);
                state.dirty = true;
            }
            else if (input == '\r' || input == '\n' || input == 13)
            {
                if (state.selected_index == 1)
                {
                    excluir_tela(state.screen);
                    excluir_objeto(state.title);
                    excluir_objeto(state.selector);
                    excluir_objeto(state.detail_frame);
                    excluir_objeto(state.showcase_player);
                    return MENU_RESULT_PLAY;
                }

                state.page = (state.selected_index == 0) ? MENU_PAGE_CREDITS : MENU_PAGE_TUTORIAL;
                esconder_objeto(state.screen, state.selector);
                state.detail_frame->position = menu_screen_to_world(new_Vector2(6, 11));
                desenhar_objeto(state.screen, state.detail_frame);
                state.dirty = true;
                state.clear_required = true;
            }
        }
        else
        {
            if (input == BACKSPACE_KEY)
            {
                state.page = MENU_PAGE_MAIN;
                esconder_objeto(state.screen, state.detail_frame);
                menu_apply_selector(&state);
                state.dirty = true;
                state.clear_required = true;
            }
        }

        if (state.dirty)
        {
            if (state.clear_required)
            {
                limpar_buffer(state.screen);
                menu_clear_terminal_area();
            }
            render(state.screen, true);
            menu_draw_overlay(&state);
            state.dirty = false;
            state.clear_required = false;
        }

        esperar(40);
    }
}

#endif