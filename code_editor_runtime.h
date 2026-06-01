#ifndef CODE_EDITOR_RUNTIME_H
#define CODE_EDITOR_RUNTIME_H

#include "code_file.h"
#include "code_binding_registry.h"
#include "game_ctx.h"

typedef struct CodeEditorSession {
    CodeFile* committed_codefile;
    CodeFile* editor_codefile;
    CodeBindingRegistry* bindings;
    bool compile_error_active;
    bool terminal_initialized;
    bool terminal_dirty;
    bool terminal_cursor_dirty;
    bool text_submit_active;
    bool last_cursor_valid;
    CodeCursor last_cursor;
    char text_submit_hint[160];
    char compile_error_message[160];
} CodeEditorSession;

typedef struct CodeEditorChrome {
    Vector2 code_origin;
    Vector2 prompt_origin;
    Vector2 error_origin;
    Vector2 footer_origin;
} CodeEditorChrome;

void codeeditor_clear_error(CodeEditorSession* session)
{
    if (session == NULL)
        return;

    session->compile_error_active = false;
    session->compile_error_message[0] = '\0';
}

void codeeditor_set_text_hint(CodeEditorSession* session, const char* hint)
{
    if (session == NULL)
        return;

    if (hint == NULL)
        hint = "";

    strncpy(session->text_submit_hint, hint, sizeof(session->text_submit_hint) - 1);
    session->text_submit_hint[sizeof(session->text_submit_hint) - 1] = '\0';
}

void codeeditor_mark_terminal_dirty(CodeEditorSession* session)
{
    if (session == NULL)
        return;

    session->terminal_dirty = true;
    session->terminal_cursor_dirty = false;
}

void codeeditor_mark_terminal_cursor_dirty(CodeEditorSession* session)
{
    if (session == NULL)
        return;

    if (!session->terminal_dirty)
        session->terminal_cursor_dirty = true;
}

void codeeditor_prepare_terminal(CodeEditorSession* session)
{
    if (session == NULL)
        return;

    session->terminal_initialized = false;
    session->terminal_dirty = true;
}

void codeeditor_init(CodeEditorSession* session, CodeFile* initial_codefile)
{
    if (session == NULL)
        return;

    session->committed_codefile = initial_codefile;
    session->editor_codefile = clone_codefile(initial_codefile);
    session->bindings = NULL;
    session->terminal_initialized = false;
    session->terminal_dirty = true;
    session->terminal_cursor_dirty = false;
    session->text_submit_active = false;
    session->last_cursor_valid = false;
    session->last_cursor = (CodeCursor){0};
    codeeditor_set_text_hint(session, "");
    codeeditor_clear_error(session);
}

void codeeditor_set_bindings(CodeEditorSession* session, CodeBindingRegistry* bindings)
{
    if (session == NULL)
        return;

    session->bindings = bindings;
}

void codeeditor_reset_editor(CodeEditorSession* session)
{
    if (session == NULL)
        return;

    reset_editor_buffer(&session->editor_codefile, session->committed_codefile);
    codeeditor_clear_error(session);
    codeeditor_set_text_hint(session, "");
    clear_codefile_compile_success_message(session->editor_codefile);
    session->terminal_dirty = true;
    session->terminal_cursor_dirty = false;
}

bool codeeditor_try_compile_and_run(CodeEditorSession* session, GameContext* ctx)
{
    if (session == NULL || ctx == NULL)
        return false;

    Vector2 null_token_pos = VETOR_NULO;
    if (codefile_has_null_token(session->editor_codefile, &null_token_pos))
    {
        snprintf(
            session->compile_error_message,
            sizeof(session->compile_error_message),
            "Compile error: null token at [%d, %d] must be replaced before run.",
            null_token_pos.x,
            null_token_pos.y
        );
        session->compile_error_active = true;
        return false;
    }

    if (session->bindings != NULL && !codebinding_registry_apply(session->bindings, session->editor_codefile, ctx))
    {
        snprintf(
            session->compile_error_message,
            sizeof(session->compile_error_message),
            "Compile error: binding points to an invalid token slot."
        );
        session->compile_error_active = true;
        return false;
    }

    commit_editor_buffer(&session->committed_codefile, session->editor_codefile);
    set_codefile_compile_success_message(session->editor_codefile, "Compilation successful.");
    codeeditor_clear_error(session);
    session->terminal_dirty = true;
    session->terminal_cursor_dirty = false;
    return true;
}

static void codeeditor_flush_input_line(void)
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {}
}

static bool codeeditor_prompt_text_submission(CodeEditorSession* session, CodeCursor cursor)
{
    if (session == NULL || session->editor_codefile == NULL)
        return false;

    char submitted[64] = {0};
    char submit_error[160] = {0};

    moveCursor(new_Vector2(1, SCREEN_SIZE_Y - 2));
    print_rgb_txt(NULL, criar_cor(220, 220, 220), new_Vector2(1, SCREEN_SIZE_Y - 2), "CMD> SUBMIT TEXT [%d,%d]", cursor.position.x, cursor.position.y);
    printf("\033[38;2;20;24;34m\033[48;2;255;245;120m");
    printf(" > ");
    fflush(stdout);

    if (fgets(submitted, sizeof(submitted), stdin) == NULL)
    {
        printf("\033[0m");
        codeeditor_set_text_hint(session, "Submission canceled.");
        return false;
    }

    size_t len = strlen(submitted);
    if (len > 0 && submitted[len - 1] == '\n')
        submitted[len - 1] = '\0';
    else
        codeeditor_flush_input_line();

    printf("\033[0m");

    if (codefile_submit_text_at_cursor(session->editor_codefile, cursor, submitted, submit_error, sizeof(submit_error)))
    {
        codeeditor_set_text_hint(session, "Submission accepted.");
        codeeditor_clear_error(session);
        return true;
    }

    if (submit_error[0] == '\0')
        snprintf(submit_error, sizeof(submit_error), "Lore guard: submission rejected.");

    snprintf(session->compile_error_message, sizeof(session->compile_error_message), "%s", submit_error);
    session->compile_error_active = true;
    codeeditor_set_text_hint(session, "Submission rejected by lore guard.");
    return false;
}

bool codeeditor_handle_text_submission_shortcut(CodeEditorSession* session, CodeCursor cursor, char input)
{
    if (session == NULL)
        return false;

    if (input != 'T')
        return false;

    session->text_submit_active = true;
    bool ok = codeeditor_prompt_text_submission(session, cursor);
    session->text_submit_active = false;
    session->terminal_dirty = true;
    session->terminal_cursor_dirty = false;
    return ok;
}

void codeeditor_destroy(CodeEditorSession* session)
{
    if (session == NULL)
        return;
    destroy_codefile(session->editor_codefile);
    destroy_codefile(session->committed_codefile);
    session->editor_codefile = NULL;
    session->committed_codefile = NULL;
    session->text_submit_active = false;
    session->terminal_cursor_dirty = false;
    session->last_cursor_valid = false;
    codeeditor_set_text_hint(session, "");
    codeeditor_clear_error(session);
}

void codeeditor_render_terminal(CodeEditorSession* session, GameContext* ctx, CodeCursor cursor, CodeEditorChrome chrome)
{
    if (session == NULL || ctx == NULL)
        return;

    if (!session->terminal_dirty && !session->terminal_cursor_dirty && session->terminal_initialized)
        return;

    Screen* screen = ctx->curScreen(ctx);

    if (session->terminal_dirty || !session->terminal_initialized || !session->last_cursor_valid)
    {
        fill_terminal_area(screen, COLOR_PRETO);
        draw_terminal_border(screen, COLOR_BRANCO);
        session->terminal_initialized = true;
    }
    else if (session->terminal_cursor_dirty)
    {
        if (session->editor_codefile != NULL)
        {
            draw_codefile_token_at(session->editor_codefile, session->last_cursor, chrome.code_origin, false);
            draw_codefile_token_at(session->editor_codefile, cursor, chrome.code_origin, true);
        }

        session->terminal_cursor_dirty = false;
        session->last_cursor = cursor;
        session->last_cursor_valid = true;
        return;
    }
    else
    {
        int clear_top = 1;
        int clear_bottom = chrome.error_origin.y + 6;
        if (session->editor_codefile != NULL)
        {
            int code_bottom = chrome.code_origin.y + session->editor_codefile->line_amount + 3;
            if (code_bottom > clear_bottom)
                clear_bottom = code_bottom;
        }
        if (clear_bottom >= screen->screen_size.y - 1)
            clear_bottom = screen->screen_size.y - 2;

        for (int y = clear_top; y <= clear_bottom; y++)
        {
            for (int x = 1; x < screen->screen_size.x - 1; x++)
                print_rgb_txt_bg(COLOR_PRETO, COLOR_PRETO, new_Vector2(x, y), " ");
        }
    }

    if (session->editor_codefile != NULL && session->editor_codefile->terminal_prompt[0] != '\0')
        draw_terminal_prompt(chrome.prompt_origin, session->editor_codefile->terminal_prompt);

    draw_codefile_at(session->editor_codefile, cursor, chrome.code_origin);

    if (session->editor_codefile != NULL
        && session->editor_codefile->compile_command[0] != '\0'
        && (session->compile_error_active || session->editor_codefile->compile_success_message[0] != '\0'))
    {
        int command_y = chrome.code_origin.y + (session->editor_codefile->line_amount > 0 ? session->editor_codefile->line_amount : 0) + 1;
        print_rgb_txt_bg(
            criar_cor(180, 240, 180),
            criar_cor(18, 34, 18),
            new_Vector2(chrome.code_origin.x, command_y),
            "%s",
            session->editor_codefile->compile_command
        );
    }

    if (session->editor_codefile != NULL && session->editor_codefile->compile_success_message[0] != '\0')
    {
        int success_y = chrome.code_origin.y + (session->editor_codefile->line_amount > 0 ? session->editor_codefile->line_amount : 0) + 2;
        print_rgb_txt_bg(
            criar_cor(30, 255, 120),
            criar_cor(8, 30, 16),
            new_Vector2(chrome.code_origin.x, success_y),
            "%s",
            session->editor_codefile->compile_success_message
        );
    }

    if (session->compile_error_active)
    {
        int error_y = chrome.code_origin.y + (session->editor_codefile != NULL ? session->editor_codefile->line_amount : 0) + 2;
        print_rgb_txt_bg(
            criar_cor(255, 220, 220),
            criar_cor(120, 20, 20),
            new_Vector2(chrome.code_origin.x, error_y),
            "%s",
            session->compile_error_message
        );
    }

    print_rgb_txt_bg(
        criar_cor(240, 240, 240),
        criar_cor(70, 70, 70),
        chrome.footer_origin,
        "Mode:%s | WASD cursor | Backspace clear | T submit text | R run | 1-5 switch | Q quit",
        "TERMINAL"
    );

    if (session->text_submit_hint[0] != '\0')
    {
        print_rgb_txt_bg(
            criar_cor(150, 240, 255),
            criar_cor(12, 28, 46),
            new_Vector2(chrome.footer_origin.x, chrome.footer_origin.y - 1),
            "%s",
            session->text_submit_hint
        );
    }

    session->terminal_dirty = false;
    session->terminal_cursor_dirty = false;
    session->last_cursor = cursor;
    session->last_cursor_valid = true;
}

#endif