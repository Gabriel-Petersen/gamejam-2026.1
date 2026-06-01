#ifndef CODE_TOKEN_H
#define CODE_TOKEN_H

#include "include/graphycs.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

typedef enum CodeTokenType { 
    TOKEN_IMMUTABLE, TOKEN_BOOL, TOKEN_NUMBER, TOKEN_OPERATOR, TOKEN_LABEL, TOKEN_NULL, TOKEN_INDENT
} CodeTokenType;

typedef struct CodeToken {
    char string[20];
    void* target_ptr; // futuramente: ponteiro tipado. Por hora, deixa assim
    CodeTokenType type;
    CodeTokenType slot_type;
    Color fwd_color;
} CodeToken;

typedef struct CodeLine {
    CodeToken* tokens;
    uint16_t capacity;
    uint16_t size;
} CodeLine;

static int codetoken_is_operator_text(const char* text)
{
    static const char* ops[] = {
        "==", "!=", "<", ">", "<=", ">=", "&&", "||"
    };

    if (text == NULL)
        return 0;

    for (int i = 0; i < (int)(sizeof(ops) / sizeof(ops[0])); i++)
    {
        if (strcmp(text, ops[i]) == 0)
            return 1;
    }

    return 0;
}

static int codetoken_is_number_text(const char* text)
{
    if (text == NULL || text[0] == '\0')
        return 0;

    for (int idx = 0; text[idx] != '\0'; idx++)
    {
        if (!isdigit((unsigned char)text[idx]))
            return 0;
    }

    return 1;
}

static int codetoken_is_label_text(const char* text)
{
    if (text == NULL || text[0] == '\0')
        return 0;

    if (!(isalpha((unsigned char)text[0]) || text[0] == '_'))
        return 0;

    for (int i = 1; text[i] != '\0'; i++)
    {
        if (!(isalnum((unsigned char)text[i]) || text[i] == '_'))
            return 0;
    }

    return 1;
}

static int codetoken_classify_text(const char* text, CodeTokenType* out_type)
{
    if (text == NULL || out_type == NULL)
        return 0;

    if (strcmp(text, "_") == 0)
    {
        *out_type = TOKEN_NULL;
        return 1;
    }

    if (strcmp(text, "true") == 0 || strcmp(text, "false") == 0)
    {
        *out_type = TOKEN_BOOL;
        return 1;
    }

    if (codetoken_is_number_text(text))
    {
        *out_type = TOKEN_NUMBER;
        return 1;
    }

    if (codetoken_is_operator_text(text))
    {
        *out_type = TOKEN_OPERATOR;
        return 1;
    }

    if (codetoken_is_label_text(text))
    {
        *out_type = TOKEN_LABEL;
        return 1;
    }

    return 0;
}

static CodeTokenType codetoken_infer_slot_type(const CodeToken* token)
{
    if (token == NULL)
        return TOKEN_IMMUTABLE;

    if (token->slot_type != TOKEN_IMMUTABLE)
        return token->slot_type;

    if (strcmp(token->string, "true") == 0 || strcmp(token->string, "false") == 0)
        return TOKEN_BOOL;

    if (codetoken_is_number_text(token->string))
        return TOKEN_NUMBER;

    if (codetoken_is_operator_text(token->string))
        return TOKEN_OPERATOR;

    if (token->type == TOKEN_LABEL)
        return TOKEN_LABEL;

    return TOKEN_IMMUTABLE;
}

static Color codetoken_default_color_for_type(CodeTokenType type)
{
    switch (type)
    {
    case TOKEN_BOOL:
        return criar_cor(180, 120, 220);
    case TOKEN_NUMBER:
        return criar_cor(90, 220, 110);
    case TOKEN_OPERATOR:
        return criar_cor(220, 90, 90);
    case TOKEN_LABEL:
        return criar_cor(90, 150, 230);
    case TOKEN_NULL:
        return criar_cor(160, 160, 160);
    case TOKEN_INDENT:
        return COLOR_BRANCO;
    default:
        return COLOR_BRANCO;
    }
}

static int codetoken_type_accepts_text(CodeTokenType slot_type, CodeTokenType submitted_type)
{
    if (slot_type == TOKEN_IMMUTABLE || slot_type == TOKEN_INDENT)
        return 0;

    if (slot_type == TOKEN_NULL)
        return submitted_type != TOKEN_IMMUTABLE && submitted_type != TOKEN_INDENT;

    return slot_type == submitted_type || submitted_type == TOKEN_NULL;
}

static int codetoken_trim_copy(const char* raw_text, char* out_trimmed, size_t out_size)
{
    if (raw_text == NULL || out_trimmed == NULL || out_size == 0)
        return 0;

    size_t raw_len = strlen(raw_text);
    size_t start = 0;
    while (start < raw_len && isspace((unsigned char)raw_text[start]))
        start++;

    size_t end = raw_len;
    while (end > start && isspace((unsigned char)raw_text[end - 1]))
        end--;

    size_t trimmed_len = end - start;
    if (trimmed_len >= out_size)
        trimmed_len = out_size - 1;

    memcpy(out_trimmed, raw_text + start, trimmed_len);
    out_trimmed[trimmed_len] = '\0';
    return trimmed_len > 0;
}

static int codetoken_submit_text(CodeToken* token, const char* raw_text, char* out_error, size_t out_error_size)
{
    if (token == NULL || raw_text == NULL)
        return 0;

    CodeTokenType slot_type = codetoken_infer_slot_type(token);

    if (slot_type == TOKEN_IMMUTABLE || token->type == TOKEN_INDENT)
    {
        if (out_error != NULL && out_error_size > 0)
            snprintf(out_error, out_error_size, "Lore guard: this token is not editable.");
        return 0;
    }

    char submitted[sizeof(token->string)];
    if (!codetoken_trim_copy(raw_text, submitted, sizeof(submitted)))
    {
        if (out_error != NULL && out_error_size > 0)
            snprintf(out_error, out_error_size, "Lore guard: empty submission is not allowed.");
        return 0;
    }

    CodeTokenType submitted_type;
    if (!codetoken_classify_text(submitted, &submitted_type))
    {
        if (out_error != NULL && out_error_size > 0)
            snprintf(out_error, out_error_size, "Compile error: token '%s' is not valid in this slot.", submitted);
        return 0;
    }

    if (slot_type == TOKEN_BOOL && submitted_type == TOKEN_NUMBER && strcmp(submitted, "0") != 0 && strcmp(submitted, "1") != 0)
        submitted_type = TOKEN_IMMUTABLE;

    if (slot_type == TOKEN_BOOL)
    {
        if (!(submitted_type == TOKEN_BOOL || (submitted_type == TOKEN_NUMBER && (strcmp(submitted, "0") == 0 || strcmp(submitted, "1") == 0))))
        {
            if (out_error != NULL && out_error_size > 0)
                snprintf(out_error, out_error_size, "Compile error: this slot accepts only true/false or 0/1.");
            return 0;
        }

        strncpy(token->string, submitted, sizeof(token->string) - 1);
        token->string[sizeof(token->string) - 1] = '\0';
        token->type = TOKEN_BOOL;
        token->slot_type = TOKEN_BOOL;
        token->fwd_color = codetoken_default_color_for_type(TOKEN_BOOL);
        if (out_error != NULL && out_error_size > 0)
            out_error[0] = '\0';
        return 1;
    }

    if (slot_type == TOKEN_NUMBER)
    {
        if (submitted_type != TOKEN_NUMBER)
        {
            if (out_error != NULL && out_error_size > 0)
                snprintf(out_error, out_error_size, "Compile error: this slot accepts only pure numbers.");
            return 0;
        }

        strncpy(token->string, submitted, sizeof(token->string) - 1);
        token->string[sizeof(token->string) - 1] = '\0';
        token->type = TOKEN_NUMBER;
        token->slot_type = TOKEN_NUMBER;
        token->fwd_color = codetoken_default_color_for_type(TOKEN_NUMBER);
        if (out_error != NULL && out_error_size > 0)
            out_error[0] = '\0';
        return 1;
    }

    if (slot_type == TOKEN_OPERATOR)
    {
        if (submitted_type != TOKEN_OPERATOR)
        {
            if (out_error != NULL && out_error_size > 0)
                snprintf(out_error, out_error_size, "Compile error: this slot accepts only comparison operators.");
            return 0;
        }

        strncpy(token->string, submitted, sizeof(token->string) - 1);
        token->string[sizeof(token->string) - 1] = '\0';
        token->type = TOKEN_OPERATOR;
        token->slot_type = TOKEN_OPERATOR;
        token->fwd_color = codetoken_default_color_for_type(TOKEN_OPERATOR);
        if (out_error != NULL && out_error_size > 0)
            out_error[0] = '\0';
        return 1;
    }

    if (slot_type == TOKEN_LABEL)
    {
        if (submitted_type != TOKEN_LABEL)
        {
            if (out_error != NULL && out_error_size > 0)
                snprintf(out_error, out_error_size, "Compile error: this slot accepts only labels.");
            return 0;
        }

        strncpy(token->string, submitted, sizeof(token->string) - 1);
        token->string[sizeof(token->string) - 1] = '\0';
        token->type = TOKEN_LABEL;
        token->slot_type = TOKEN_LABEL;
        token->fwd_color = codetoken_default_color_for_type(TOKEN_LABEL);
        if (out_error != NULL && out_error_size > 0)
            out_error[0] = '\0';
        return 1;
    }

    if (!codetoken_type_accepts_text(slot_type, submitted_type))
    {
        if (out_error != NULL && out_error_size > 0)
            snprintf(out_error, out_error_size, "Compile error: this token slot is not editable.");
        return 0;
    }

    strncpy(token->string, submitted, sizeof(token->string) - 1);
    token->string[sizeof(token->string) - 1] = '\0';
    token->type = submitted_type;
    token->slot_type = submitted_type;
    token->fwd_color = codetoken_default_color_for_type(submitted_type);
    if (out_error != NULL && out_error_size > 0)
        out_error[0] = '\0';
    return 1;
}

CodeLine* create_codeline()
{
    CodeLine* line = (CodeLine*)malloc(sizeof(CodeLine));
    line->capacity = 4;
    line->size = 0;
    line->tokens = (CodeToken*)malloc(line->capacity * sizeof(CodeToken));

    return line;
}

void destroy_codeline(CodeLine* line)
{
    if (line == NULL)
        return;

    free(line->tokens);
    free(line);
}

void push_back_token(CodeLine* line, CodeToken tk)
{
    if (line->size == line->capacity)
    {
        uint16_t new_capacity = (uint16_t)(line->capacity * 2);
        CodeToken* resized = (CodeToken*)realloc(
            line->tokens, new_capacity * sizeof(CodeToken)
        );

        line->tokens = resized;
        line->capacity = new_capacity;
    }

    line->tokens[line->size++] = tk;
}

CodeToken create_immutable_token(const char* text, Color color)
{
    CodeToken token = {0};
    strncpy(token.string, text, sizeof(token.string) - 1);
    token.target_ptr = NULL;
    token.type = TOKEN_IMMUTABLE;
    token.slot_type = TOKEN_IMMUTABLE;
    token.fwd_color = color;
    return token;
}

CodeToken create_indent_token(uint16_t width)
{
    CodeToken token = {0};
    if (width >= sizeof(token.string))
        width = (uint16_t)(sizeof(token.string) - 1);

    memset(token.string, ' ', width);
    token.string[width] = '\0';
    token.target_ptr = NULL;
    token.type = TOKEN_INDENT;
    token.slot_type = TOKEN_INDENT;
    token.fwd_color = COLOR_BRANCO;
    return token;
}

void set_token_literal(CodeToken* tk, const char* text, CodeTokenType type, Color color)
{
    if (tk == NULL || text == NULL)
        return;

    strncpy(tk->string, text, sizeof(tk->string) - 1);
    tk->string[sizeof(tk->string) - 1] = '\0';
    tk->type = type;
    tk->fwd_color = color;
}

void push_immutable_token(CodeLine* line, const char* text, Color color)
{
    push_back_token(line, create_immutable_token(text, color));
}

void push_indent_token(CodeLine* line, uint16_t width)
{
    push_back_token(line, create_indent_token(width));
}

#endif