 #define DEBUG_ENABLE

#ifdef DEBUG_ENABLE
#define DEBUG_LOG(...) do { } while (0)
#else
#define DEBUG_LOG(...) printf(__VA_ARGS__)
#endif

#include "menu.h"
#include "level.h"
#include "levels/level_1.h"
#include "levels/level_2.h"

int main()
{
    if (menu_run() != MENU_RESULT_PLAY)
        return 0;

    GameContext* ctx = create_game_ctx();
    if (ctx == NULL)
        return 1;

    const LevelDefinition* campaign_levels[] = {
        level1_get_definition(),
        level2_get_definition()
    };
    const int total_levels = (int)(sizeof(campaign_levels) / sizeof(campaign_levels[0]));
    int current_level_index = 0;

    while (current_level_index < total_levels)
    {
        LevelContext level_ctx = level_create_context(ctx, current_level_index, total_levels, NULL);
        LevelInstance level;
        level_instance_reset(&level, &level_ctx, campaign_levels[current_level_index], NULL);

        if (!level_start(&level, &level_ctx))
        {
            freeGameCtx(ctx);
            return 1;
        }

        while (level.running)
        {
            char input = ler_teclado();
            if (input == 'Q')
            {
                level_request_exit(&level, LEVEL_EXIT_QUIT, -1);
                break;
            }

            level_step_input(&level, &level_ctx, input);
            if (!level.running)
                break;

            level_step_tick(&level, &level_ctx);
            if (!level.running)
                break;

            level_step_draw(&level, &level_ctx);
        }

        LevelExitReason exit_reason = level.exit_reason;
        game_clear_all_screens(ctx);
        int next_level_index = level.next_level_index;
        level_stop(&level, &level_ctx);

        if (exit_reason == LEVEL_EXIT_COMPLETED)
        {
            if (next_level_index < 0)
                next_level_index = current_level_index + 1;
            current_level_index = next_level_index;
            continue;
        }

        if (exit_reason == LEVEL_EXIT_RESTART)
            continue;

        break;
    }

    freeGameCtx(ctx);
    return 0;
}
