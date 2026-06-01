#ifndef CODE_BINDING_REGISTRY_H
#define CODE_BINDING_REGISTRY_H

#include "code_file.h"
#include "entity.h"

typedef struct CodeBindingEntry {
    Entity* entity;
    int line;
    int token;
    EntityTokenHandler on_token;
} CodeBindingEntry;

typedef struct CodeBindingRegistry {
    CodeBindingEntry* entries;
    int count;
    int capacity;
} CodeBindingRegistry;

void codebinding_registry_init(CodeBindingRegistry* registry)
{
    if (registry == NULL)
        return;

    registry->entries = NULL;
    registry->count = 0;
    registry->capacity = 0;
}

void codebinding_registry_destroy(CodeBindingRegistry* registry)
{
    if (registry == NULL)
        return;

    free(registry->entries);
    registry->entries = NULL;
    registry->count = 0;
    registry->capacity = 0;
}

static CodeToken* codebinding_registry_get_token(CodeFile* file, int line_idx, int token_idx)
{
    if (file == NULL || line_idx < 0 || token_idx < 0)
        return NULL;

    if (line_idx >= file->line_amount)
        return NULL;

    CodeLine* line = file->lines[line_idx];
    if (line == NULL || token_idx >= line->size)
        return NULL;

    return &line->tokens[token_idx];
}

bool codebinding_registry_add(CodeBindingRegistry* registry, Entity* entity, int line_idx, int token_idx, EntityTokenHandler on_token)
{
    if (registry == NULL || entity == NULL || on_token == NULL)
        return false;

    if (registry->count == registry->capacity)
    {
        int new_capacity = (registry->capacity == 0) ? 4 : registry->capacity * 2;
        CodeBindingEntry* resized = (CodeBindingEntry*)realloc(
            registry->entries,
            new_capacity * sizeof(CodeBindingEntry)
        );
        if (resized == NULL)
            return false;

        registry->entries = resized;
        registry->capacity = new_capacity;
    }

    registry->entries[registry->count++] = (CodeBindingEntry){
        .entity = entity,
        .line = line_idx,
        .token = token_idx,
        .on_token = on_token
    };

    return true;
}

const CodeBindingEntry* codebinding_registry_find(CodeBindingRegistry* registry, int line_idx, int token_idx)
{
    if (registry == NULL)
        return NULL;

    for (int i = 0; i < registry->count; i++)
    {
        CodeBindingEntry* entry = &registry->entries[i];
        if (entry->line == line_idx && entry->token == token_idx)
            return entry;
    }

    return NULL;
}

bool codebinding_registry_apply(CodeBindingRegistry* registry, CodeFile* file, GameContext* ctx)
{
    if (registry == NULL || file == NULL || ctx == NULL)
        return false;

    for (int i = 0; i < registry->count; i++)
    {
        CodeBindingEntry* entry = &registry->entries[i];
        CodeToken* token = codebinding_registry_get_token(file, entry->line, entry->token);
        if (token == NULL)
            return false;

        entry->on_token(entry->entity, token, ctx);
    }

    return true;
}

#endif