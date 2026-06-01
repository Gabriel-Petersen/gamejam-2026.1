#ifndef CODE_FILE_H
#define CODE_FILE_H

#define USE_SHORTCUTS
#include "include/graphycs_all.h"
#include "code_token.h"

typedef struct CodeCursor {
    Vector2 position;
    Color bg_color;
    Color imm_bg_color;
} CodeCursor;

typedef struct CodeFile {
    CodeLine** lines;
    int line_amount;
    int line_capacity;
    char terminal_prompt[128];
    char compile_command[160];
    char compile_success_message[160];
} CodeFile;

typedef struct Entity Entity;
typedef struct GameContext GameContext;

void entity_handle_token(Entity* ent, CodeToken* token, GameContext* ctx);

CodeToken clone_codetoken(CodeToken token)
{
    CodeToken copy = token;
    return copy;
}

CodeLine* clone_codeline(CodeLine* line)
{
    if (line == NULL)
        return NULL;

    CodeLine* copy = create_codeline();
    if (line->size > copy->capacity)
    {
        copy->capacity = line->size;
        copy->tokens = (CodeToken*)realloc(copy->tokens, copy->capacity * sizeof(CodeToken));
    }

    for (int i = 0; i < line->size; i++)
        push_back_token(copy, clone_codetoken(line->tokens[i]));

    return copy;
}

Color cursor_contrast_color(Color bg)
{
    int brightness = bg.r + bg.g + bg.b;
    if (brightness >= 384)
        return criar_cor(0, 0, 0);

    return criar_cor(255, 255, 255);
}

CodeFile* create_codefile()
{
    CodeFile* file = (CodeFile*)malloc(sizeof(CodeFile));
    file->lines = (CodeLine**)malloc(2 * sizeof(CodeLine*));
    file->line_amount = 0;
    file->line_capacity = 2;
    file->terminal_prompt[0] = '\0';
    file->compile_command[0] = '\0';
    file->compile_success_message[0] = '\0';
    return file;
}

CodeFile* clone_codefile(CodeFile* file)
{
    if (file == NULL)
        return NULL;

    CodeFile* copy = create_codefile();
    strncpy(copy->terminal_prompt, file->terminal_prompt, sizeof(copy->terminal_prompt) - 1);
    copy->terminal_prompt[sizeof(copy->terminal_prompt) - 1] = '\0';
    strncpy(copy->compile_command, file->compile_command, sizeof(copy->compile_command) - 1);
    copy->compile_command[sizeof(copy->compile_command) - 1] = '\0';
    strncpy(copy->compile_success_message, file->compile_success_message, sizeof(copy->compile_success_message) - 1);
    copy->compile_success_message[sizeof(copy->compile_success_message) - 1] = '\0';
    if (file->line_capacity > copy->line_capacity)
    {
        CodeLine** resized = (CodeLine**)realloc(copy->lines, file->line_capacity * sizeof(CodeLine*));
        copy->lines = resized;
        copy->line_capacity = file->line_capacity;
    }

    for (int i = 0; i < file->line_amount; i++)
        copy->lines[copy->line_amount++] = clone_codeline(file->lines[i]);

    return copy;
}

void destroy_codefile(CodeFile* file)
{
    if (file == NULL)
        return;

    for (int i = 0; i < file->line_amount; i++)
        destroy_codeline(file->lines[i]);

    free(file->lines);
    free(file);
}


void reset_editor_buffer(CodeFile** editor_codefile, CodeFile* committed_codefile)
{
    if (editor_codefile == NULL || committed_codefile == NULL)
        return;

    if (*editor_codefile != NULL)
        destroy_codefile(*editor_codefile);
    *editor_codefile = clone_codefile(committed_codefile);
}

void commit_editor_buffer(CodeFile** committed_codefile, CodeFile* editor_codefile)
{
    if (committed_codefile == NULL || editor_codefile == NULL)
        return;

    if (*committed_codefile != NULL)
        destroy_codefile(*committed_codefile);
    *committed_codefile = clone_codefile(editor_codefile);
}

void set_codefile_terminal_prompt(CodeFile* file, const char* prompt)
{
    if (file == NULL)
        return;

    if (prompt == NULL)
        prompt = "";

    strncpy(file->terminal_prompt, prompt, sizeof(file->terminal_prompt) - 1);
    file->terminal_prompt[sizeof(file->terminal_prompt) - 1] = '\0';
}

void set_codefile_compile_command(CodeFile* file, const char* command)
{
    if (file == NULL)
        return;

    if (command == NULL)
        command = "";

    strncpy(file->compile_command, command, sizeof(file->compile_command) - 1);
    file->compile_command[sizeof(file->compile_command) - 1] = '\0';
}

void set_codefile_compile_success_message(CodeFile* file, const char* message)
{
    if (file == NULL)
        return;

    if (message == NULL)
        message = "";

    strncpy(file->compile_success_message, message, sizeof(file->compile_success_message) - 1);
    file->compile_success_message[sizeof(file->compile_success_message) - 1] = '\0';
}

void clear_codefile_compile_feedback(CodeFile* file)
{
    if (file == NULL)
        return;

    file->compile_success_message[0] = '\0';
}

void clear_codefile_compile_success_message(CodeFile* file)
{
    if (file == NULL)
        return;

    file->compile_success_message[0] = '\0';
}

CodeToken* codefile_get_token_at_cursor(CodeFile* file, CodeCursor cursor)
{
    if (file == NULL)
        return NULL;

    int line_idx = cursor.position.y;
    int token_idx = cursor.position.x;
    if (line_idx < 0 || token_idx < 0 || line_idx >= file->line_amount)
        return NULL;

    CodeLine* line = file->lines[line_idx];
    if (line == NULL || token_idx >= line->size)
        return NULL;

    return &line->tokens[token_idx];
}

bool codefile_submit_text_at_cursor(CodeFile* file, CodeCursor cursor, const char* raw_text, char* out_error, size_t out_error_size)
{
    CodeToken* token = codefile_get_token_at_cursor(file, cursor);
    if (token == NULL)
    {
        if (out_error != NULL && out_error_size > 0)
            snprintf(out_error, out_error_size, "Lore guard: invalid cursor target.");
        return false;
    }

    return codetoken_submit_text(token, raw_text, out_error, out_error_size) != 0;
}

bool codefile_has_null_token(CodeFile* file, Vector2* out_pos)
{
    if (file == NULL)
        return false;

    for (int line_idx = 0; line_idx < file->line_amount; line_idx++)
    {
        CodeLine* line = file->lines[line_idx];
        if (line == NULL)
            continue;

        for (int token_idx = 0; token_idx < line->size; token_idx++)
        {
            if (line->tokens[token_idx].type == TOKEN_NULL)
            {
                if (out_pos != NULL)
                    *out_pos = new_Vector2(token_idx, line_idx);
                return true;
            }
        }
    }

    return false;
}

void apply_code_bindings_to_world(CodeFile* codefile, GameContext* ctx)
{
    if (codefile == NULL || ctx == NULL)
        return;

    for (int line_idx = 0; line_idx < codefile->line_amount; line_idx++)
    {
        CodeLine* line = codefile->lines[line_idx];
        if (line == NULL)
            continue;

        for (int token_idx = 0; token_idx < line->size; token_idx++)
        {
            CodeToken* token = &line->tokens[token_idx];
            Entity* target = (Entity*)token->target_ptr;
            if (target != NULL)
                entity_handle_token(target, token, ctx);
        }
    }
}

CodeLine* push_back_empty_codeline(CodeFile* file)
{
    if (file->line_amount == file->line_capacity)
    {
        int new_capacity = file->line_capacity * 2;
        CodeLine** resized = (CodeLine**)realloc(
            file->lines, new_capacity * sizeof(CodeLine*)
        );

        file->lines = resized;
        file->line_capacity = new_capacity;
    }

    CodeLine* line = create_codeline();
    file->lines[file->line_amount++] = line;
    return line;
}

void draw_codetoken(CodeToken* token, bool highlighted, Vector2 pos, CodeCursor* cursor)
{
    if (token->type == TOKEN_INDENT)
    {
        print_rgb_txt(NULL, token->fwd_color, pos, "%s", token->string);
        return;
    }

    Color color = COLOR_PRETO, txt_color = token->fwd_color;
    if (highlighted)
    {
        color = token->type == TOKEN_IMMUTABLE ? cursor->imm_bg_color : cursor->bg_color;
        txt_color = cursor_contrast_color(color);
    }

    print_rgb_txt_bg(txt_color, color, pos, "%s", token->string);
}

static Vector2 codefile_token_screen_pos(CodeFile* file, CodeCursor cursor, Vector2 origin)
{
    Vector2 pos = origin;
    if (file == NULL)
        return pos;

    if (cursor.position.y < 0 || cursor.position.y >= file->line_amount)
        return pos;

    CodeLine* line = file->lines[cursor.position.y];
    if (line == NULL)
        return pos;

    pos.y += cursor.position.y;
    for (int token_idx = 0; token_idx < cursor.position.x && token_idx < line->size; token_idx++)
    {
        pos.x += (int)strlen(line->tokens[token_idx].string);
        if (token_idx < line->size - 1)
            pos.x += 1;
    }

    return pos;
}

static void draw_codefile_token_at(CodeFile* file, CodeCursor cursor, Vector2 origin, bool highlighted)
{
    if (file == NULL)
        return;

    if (cursor.position.y < 0 || cursor.position.y >= file->line_amount)
        return;

    CodeLine* line = file->lines[cursor.position.y];
    if (line == NULL || cursor.position.x < 0 || cursor.position.x >= line->size)
        return;

    CodeToken* token = &line->tokens[cursor.position.x];
    Vector2 token_pos = codefile_token_screen_pos(file, cursor, origin);
    draw_codetoken(token, highlighted, token_pos, &cursor);
}

void draw_codefile(CodeFile* file, CodeCursor cursor)
{
    for (int line_idx = 0; line_idx < file->line_amount; line_idx++)
    {
        CodeLine* line = file->lines[line_idx];
        if (line == NULL)
            continue;

        int x = 0;
        for (int token_idx = 0; token_idx < line->size; token_idx++)
        {
            CodeToken* token = &line->tokens[token_idx];
            bool highlighted = compare_vector(cursor.position, nv2(token_idx, line_idx));
            Vector2 token_pos = nv2(x, line_idx);

            draw_codetoken(token, highlighted, token_pos, &cursor);
            x += (int)strlen(token->string);

            if (token_idx < line->size - 1)
            {
                print_rgb_txt(NULL, COLOR_BRANCO, new_Vector2(x, line_idx), " ");
                x += 1;
            }
        }
    }
}

void draw_codefile_at(CodeFile* file, CodeCursor cursor, Vector2 origin)
{
    for (int line_idx = 0; line_idx < file->line_amount; line_idx++)
    {
        CodeLine* line = file->lines[line_idx];
        if (line == NULL)
            continue;

        int x = origin.x;
        int y = origin.y + line_idx;
        for (int token_idx = 0; token_idx < line->size; token_idx++)
        {
            CodeToken* token = &line->tokens[token_idx];
            bool highlighted = compare_vector(cursor.position, nv2(token_idx, line_idx));
            Vector2 token_pos = nv2(x, y);

            draw_codetoken(token, highlighted, token_pos, &cursor);
            x += (int)strlen(token->string);

            if (token_idx < line->size - 1)
            {
                print_rgb_txt_bg(COLOR_PRETO, COLOR_PRETO, new_Vector2(x, y), " ");
                x += 1;
            }
        }
    }
}

int codefile_render_width(CodeFile* file)
{
    int max_width = 0;
    if (file == NULL)
        return max_width;

    for (int line_idx = 0; line_idx < file->line_amount; line_idx++)
    {
        CodeLine* line = file->lines[line_idx];
        if (line == NULL)
            continue;

        int width = 0;
        for (int token_idx = 0; token_idx < line->size; token_idx++)
        {
            CodeToken* token = &line->tokens[token_idx];
            width += (int)strlen(token->string);
            if (token_idx < line->size - 1)
                width += 1;
        }

        if (width > max_width)
            max_width = width;
    }

    return max_width;
}

void clear_codefile_render_area(CodeFile* file, int extra_lines, int width)
{
    int lines = (file != NULL ? file->line_amount : 0) + extra_lines;
    if (lines < 1)
        lines = 1;
    if (width < 1)
        width = 1;

    for (int line_idx = 0; line_idx < lines; line_idx++)
    {
        moveCursor(new_Vector2(0, line_idx));
        for (int x = 0; x < width; x++)
            print_rgb_txt_bg(COLOR_PRETO, COLOR_PRETO, new_Vector2(x, line_idx), " ");
    }
}

void draw_terminal_border(Screen* screen, Color border_color)
{
    if (screen == NULL)
        return;

    int width = screen->screen_size.x;
    int height = screen->screen_size.y;

    for (int x = 0; x < width; x++)
    {
        print_rgb_txt_bg(COLOR_PRETO, border_color, new_Vector2(x, 0), " ");
        print_rgb_txt_bg(COLOR_PRETO, border_color, new_Vector2(x, height - 1), " ");
    }

    for (int y = 0; y < height; y++)
    {
        print_rgb_txt_bg(COLOR_PRETO, border_color, new_Vector2(0, y), " ");
        print_rgb_txt_bg(COLOR_PRETO, border_color, new_Vector2(width - 1, y), " ");
    }
}

void fill_terminal_area(Screen* screen, Color color)
{
    if (screen == NULL)
        return;

    for (int y = 0; y < screen->screen_size.y; y++)
        for (int x = 0; x < screen->screen_size.x; x++)
            print_rgb_txt_bg(COLOR_PRETO, color, new_Vector2(x, y), " ");
}

void draw_terminal_prompt(Vector2 origin, const char* prompt)
{
    if (prompt == NULL)
        prompt = "";

    print_rgb_txt_bg(criar_cor(120, 255, 120), COLOR_PRETO, origin, "%s", prompt);
}

void clamp_code_cursor(CodeCursor* cursor, CodeFile* file)
{
    if (file->line_amount <= 0)
    {
        cursor->position = VETOR_NULO;
        return;
    }

    if (cursor->position.y < 0)
        cursor->position.y = 0;
    if (cursor->position.y >= file->line_amount)
        cursor->position.y = file->line_amount - 1;

    CodeLine* line = file->lines[(int)cursor->position.y];
    if (line == NULL || line->size <= 0)
    {
        cursor->position.x = 0;
        return;
    }

    if (cursor->position.x < 0)
        cursor->position.x = 0;
    if (cursor->position.x >= line->size)
        cursor->position.x = line->size - 1;
}

int resolve_selectable_token_index(CodeLine* line, int desired_index, int fallback_index, int step)
{
    if (line == NULL || line->size <= 0)
        return 0;

    if (desired_index < 0)
        desired_index = 0;
    if (desired_index >= line->size)
        desired_index = line->size - 1;

    if (fallback_index < 0)
        fallback_index = 0;
    if (fallback_index >= line->size)
        fallback_index = line->size - 1;

    if (line->tokens[desired_index].type != TOKEN_INDENT)
        return desired_index;

    int directions[2] = {
        (step >= 0) ? 1 : -1,
        (step >= 0) ? -1 : 1
    };

    for (int dir_idx = 0; dir_idx < 2; dir_idx++)
    {
        int direction = directions[dir_idx];
        for (int idx = desired_index + direction; idx >= 0 && idx < line->size; idx += direction)
        {
            if (line->tokens[idx].type != TOKEN_INDENT)
                return idx;
        }
    }

    if (line->tokens[fallback_index].type != TOKEN_INDENT)
        return fallback_index;

    for (int dir_idx = 0; dir_idx < 2; dir_idx++)
    {
        int direction = directions[dir_idx];
        for (int idx = fallback_index + direction; idx >= 0 && idx < line->size; idx += direction)
        {
            if (line->tokens[idx].type != TOKEN_INDENT)
                return idx;
        }
    }

    return fallback_index;
}

void move_code_cursor(CodeCursor* cursor, CodeFile* file, int dx, int dy)
{
    int previous_x = cursor->position.x;
    cursor->position.x += dx;
    cursor->position.y += dy;
    clamp_code_cursor(cursor, file);

    if (file->line_amount <= 0)
        return;

    CodeLine* line = file->lines[(int)cursor->position.y];
    if (line == NULL || line->size <= 0)
        return;

    cursor->position.x = resolve_selectable_token_index(line, cursor->position.x, previous_x, dx);
    clamp_code_cursor(cursor, file);
}

#endif