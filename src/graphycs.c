#include "../include/graphycs.h"
#define _USE_MATH_DEFINES
#include <math.h>
#define COR_NULA (Color){-10, -10, -10}

static Obj_Linked_List* criar_lista_objetos ()
{
    Obj_Linked_List* ol = (Obj_Linked_List*)malloc(sizeof(Obj_Linked_List));
    ol->inicio = NULL;
    ol->qtd = 0;
    return ol;
}

static void remover_da_lista (Obj_Linked_List* ol, ObjNode* nodo)
{
    if (nodo == NULL)
    {
        printf("Erro interno, tentando removendo nodo nulo!!!\n");
        return;
    }

    if (nodo->ant != NULL)
        nodo->ant->prox = nodo->prox;
    else
        ol->inicio = nodo->prox;
    
    if (nodo->prox != NULL)
        nodo->prox->ant = nodo->ant;
    ol->qtd--;
    free(nodo);
}

static void incluir_na_lista (Obj_Linked_List* obj_list, void* obj_ptr, char obj_type)
{
    ObjNode* novo_nodo = (ObjNode*)malloc(sizeof(ObjNode));
    if (obj_list->inicio != NULL)
        obj_list->inicio->ant = novo_nodo;
    novo_nodo->prox = obj_list->inicio;
    novo_nodo->ant = NULL;
    novo_nodo->obj_type = obj_type;
    if (obj_type == 'o')
    {
        novo_nodo->obj_ref = (Objeto*)obj_ptr;
        ((Objeto*)obj_ptr)->ref_node = novo_nodo;
    }
    else if (obj_type == 'c')
    {
        novo_nodo->obj_ref = (ObjetoComplexo*)obj_ptr;
        ((ObjetoComplexo*)obj_ptr)->ref_node = novo_nodo;
    }
    else
    {
        printf("ERRO: Objeto possui char de flag = %c - só é permitido 'o' ou 'c'\n", obj_type);
    }
    obj_list->inicio = novo_nodo;
    obj_list->qtd++;
}

#if defined(_WIN32) || defined(_WIN64)
// ================= WINDOWS =================
#include <windows.h>
char ler_teclado(void) {
    char ultima = '\0';
    while (_kbhit())
        ultima = (char)toupper((unsigned char)getch());
    return ultima;
}

void esperar(int millis)
{
    Sleep((unsigned long)millis);
}

#else
// ================= LINUX =================
static int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

void esperar(int millis)
{
    usleep((useconds_t)millis * 1000);
}

char ler_teclado(void) {
    char ultima = '\0';
    while (kbhit()) {
        int ch = getchar();
        if (ch == EOF)
            break;
        ultima = (char)toupper((unsigned char)ch);
    }
    return ultima;
}
#endif

void print_vector(Vector2 v, char* name)
{
    printf("%s: (%d, %d)\n", name, v.x, v.y);
}

static int next_anim(Animation* anim)
{
    return (anim->frame_atual + 1 == anim->qtd_frames) ? 0 : anim->frame_atual + 1;
}

static Color converter_ABGR_para_Color(uint32_t abgr) 
{
    uint8_t alpha = (abgr >> 24) & 0xFF;
    if(alpha == 0)
        return COLOR_PRETO;
    
    uint8_t r = abgr & 0xFF; // Obtém o vermelho
    uint8_t g = (abgr >> 8)  & 0xFF; // Obtém o verde
    uint8_t b = (abgr >> 16) & 0xFF; // Obtém o azul

    return criar_cor(r, g, b);
}


static bool color_equals (Color c1, Color c2)
{
    bool r = c1.r == c2.r;
    bool g = c1.g == c2.g;
    bool b = c1.b == c2.b;
    return r && g && b;
}

Color criar_cor(int r, int g, int b) {
    return (Color){r, g, b};
}

#ifndef min
    #define min(a,b) (a < b ? a : b)
#endif
#ifndef max
#define max(a,b) (a > b ? a : b)
#endif

Color aplicar_filtro (Color original, Color filtro)
{
    return criar_cor(
        min (255, original.r * filtro.r / 255),
        min (255, original.g * filtro.g / 255),
        min (255, original.b * filtro.b / 255)
    );
}

Color somar_cor (Color original, Color soma)
{
    return criar_cor (
        min (255, original.r + soma.r),
        min (255, original.g + soma.g),
        min (255, original.b + soma.b)
    );
}

Vector2 new_Vector2 (int x, int y)
{
    return (Vector2){x, y};
}

Vector2 produto_vetor_escalar (Vector2 vet, float escalar)
{
    return new_Vector2(
        vet.x * (int)round(escalar),
        vet.y * (int)round(escalar)
    );
}

Vector2 vector_sum (Vector2 v1, Vector2 v2) {
    return new_Vector2 (
        v1.x + v2.x, 
        v1.y + v2.y
    );
}

Vector2 vector_subtr(Vector2 v1, Vector2 v2)
{
    return new_Vector2 (
        v1.x - v2.x, 
        v1.y - v2.y
    ); 
}

Vector2 reverse_vector (Vector2 v){
    return new_Vector2 (-v.x, -v.y);
}

Vector2 centro_objeto_complexo(ObjetoComplexo* obj)
{
    return produto_vetor_escalar(obj->size, 0.5f);
}

Vector2 get_complexo_abs_pixel_pos(ObjetoComplexo* obj, int frame, int index)
{
    return new_Vector2(
        obj->frames[frame]->info[index].position.x + obj->position.x + obj->pivot_frames[frame].x,
        obj->frames[frame]->info[index].position.y + obj->position.y + obj->pivot_frames[frame].y
    );
}

static Vector2 aplicar_matriz_rot (Vector2 Pixel_pos, Vector2 rot_pivo, float graus)
{
    float radianos = graus * M_PI/180.0;
    float x_rel = Pixel_pos.x - rot_pivo.x;
    float y_rel = -(Pixel_pos.y - rot_pivo.y);

    float x_rotacionado = x_rel * cos(radianos) - y_rel * sin(radianos);
    float y_rotacionado = x_rel * sin(radianos) + y_rel * cos(radianos);

    return new_Vector2(round(x_rotacionado + rot_pivo.x), round(-y_rotacionado + rot_pivo.y));
}

Pixel criar_pixel (Color cor, Vector2 pos)
{
    Pixel p;
    p.cor = cor;
    p.position = pos;

    return p;
}

static Pixel_Stack* criar_pilha()
{
    Pixel_Stack* stack = (Pixel_Stack*)malloc(sizeof(Pixel_Stack));
    stack->topo = NULL;
    return stack;
}

static Pixel_Node* criar_nodo (Pixel p)
{
    Pixel_Node* n = (Pixel_Node*)malloc(sizeof(Pixel_Node));
    if (n) 
    {
        n->pixel = p;
        n->anterior = NULL;
    }
    return n;
}

static void add_pixel (Pixel_Stack* stack, Pixel p)
{
    Pixel_Node* x = criar_nodo(p);
    x->anterior = stack->topo;
    stack->topo = x;
}

void mover_tela (Screen* s, Vector2 direction)
{
    for (int i = 0; i < s->screen_size.y; i++) for (int j = 0; j < s->screen_size.x; j++)
    {
        Pixel_Stack* stack = s->pixeis[i][j];
        while (stack->topo->anterior != NULL)
        {
            desempilhar_pixel(stack);
        }
    }

    s->position = vector_sum(s->position, direction);

    ObjNode* atual = s->obj_list->inicio;
    while (atual != NULL)
    {
        if (atual->obj_type == 'o')
        {
            Objeto* obj = (Objeto*)atual->obj_ref;
            obj->renderizado = false;
            desenhar_objeto(s, obj);
            obj->renderizado = true;
        }
        else if (atual->obj_type == 'c')
        {
            ObjetoComplexo* objC = (ObjetoComplexo*)atual->obj_ref;
            objC->renderizado = false;
            desenhar_objeto_complexo(s, objC);
            objC->renderizado = true;
        }
        atual = atual->prox;
    }

    limpar_buffer(s);
}

Vector2 get_abs_pixel_pos (Objeto* obj, int index)
{
    return vector_sum(obj->info[index].position, obj->position);
}

Vector2 centro_do_objeto(Objeto *obj) 
{
    return produto_vetor_escalar(obj->size, 0.5f);
}

Vector2 centro_da_tela(Screen* s)
{
    return new_Vector2(
        s->screen_size.x / 2,
        s->screen_size.y / 2
    );
}

Pixel desempilhar_pixel (Pixel_Stack* stack)
{
    Pixel_Node* x = stack->topo;
    Pixel noPixel = criar_pixel(COLOR_PRETO, VETOR_NULO);
    if (x == NULL) return noPixel;
    stack->topo = stack->topo->anterior;
    noPixel = x->pixel;
    free(x);
    return noPixel;
}

void normalizar_objeto(Objeto* obj) 
{
    if (obj->qtd_pixel <= 0)
        return;
    
    int min_x = obj->info[0].position.x;
    int min_y = obj->info[0].position.y;
    
    for (int i = 1; i < obj->qtd_pixel; i++) 
    {
        if (obj->info[i].position.x < min_x) min_x = obj->info[i].position.x;
        if (obj->info[i].position.y < min_y) min_y = obj->info[i].position.y;
    }
    
    for (int i = 0; i < obj->qtd_pixel; i++) 
    {
        obj->info[i].position.x -= min_x;
        obj->info[i].position.y -= min_y;
    }
}

void centralizar_objeto(Objeto* obj)
{
    if (obj->qtd_pixel <= 0)
        return;

    Vector2 min = new_Vector2(INT_MAX, INT_MAX);
    Vector2 max = new_Vector2(INT_MIN, INT_MIN);

    for (int i = 0; i < obj->qtd_pixel; i++) {
        Vector2 p = obj->info[i].position;
        if (p.x < min.x) min.x = p.x;
        if (p.y < min.y) min.y = p.y;
        if (p.x > max.x) max.x = p.x;
        if (p.y > max.y) max.y = p.y;
    }

    Vector2 centro = new_Vector2(
        (min.x + max.x) / 2,
        (min.y + max.y) / 2
    );

    for (int i = 0; i < obj->qtd_pixel; i++) 
        obj->info[i].position = vector_subtr(obj->info[i].position, centro);

    obj->size = vector_subtr(max, min);
}

void centralizar_objeto_complexo(ObjetoComplexo* obj)
{
    if (obj->qtd_frames <= 0)
        return;
    
    for (int i = 0; i < obj->qtd_frames; i++)
    {
        centralizar_objeto(obj->frames[i]);
    }
}

Objeto* criar_objeto_custom (Pixel* info, int qtd_pixel, bool normalizar)
{
    Objeto* o = (Objeto*)malloc(sizeof(Objeto));
    o->info = info;
    o->position = VETOR_NULO;
    o->qtd_pixel = qtd_pixel;
    o->renderizado = false;
    o->ref_node = NULL;
    Vector2 menor = info[0].position;
    Vector2 maior = VETOR_NULO;
    for (int i = 0; i < qtd_pixel; i++)
    {
        if (info[i].position.x < menor.x)
            menor.x = info[i].position.x;
        if (info[i].position.y < menor.y)
            menor.y = info[i].position.y;
        if (info[i].position.x > maior.x)
            maior.x = info[i].position.x;
        if (info[i].position.y > maior.y)
            maior.y = info[i].position.y;
    }

    Vector2 tamanho = new_Vector2(maior.x - menor.x, maior.y - menor.y);
    o->size = tamanho;
    if (normalizar)
        normalizar_objeto(o);

    return o;
}

Objeto* criar_retangulo_monocromatico (Color cor, Vector2 tamanho)
{
    Objeto* o = (Objeto*)malloc(sizeof(Objeto));
    int area = tamanho.x * tamanho.y;
    o->qtd_pixel = area;
    o->position = VETOR_NULO;
    o->renderizado = false;
    o->size = tamanho;
    o->ref_node = NULL;
    Pixel* info = (Pixel*)malloc(area*sizeof(Pixel));
    for (int i = 0; i < area; i++)
    {
        info[i].cor = cor;
        info[i].position = new_Vector2(i % tamanho.x, i / tamanho.x);
    }
    o->info = info;

    return o;
}

Objeto* criar_frame_retangular(Vector2 size, Color cor)
{
    if (size.x < 2 || size.y < 2)
        return NULL;

    int qtd_pixel = 2 * (size.x + size.y - 2);

    Pixel* info = (Pixel*)malloc(qtd_pixel * sizeof(Pixel));
    if (info == NULL) return NULL;

    int idx = 0;

    for (int x = 0; x < size.x; x++)
        info[idx++] = criar_pixel(cor, new_Vector2(x, 0));

    for (int x = 0; x < size.x; x++)
        info[idx++] = criar_pixel(cor, new_Vector2(x, size.y - 1));

    for (int y = 1; y < size.y - 1; y++)
    {
        info[idx++] = criar_pixel(cor, new_Vector2(0, y));
        info[idx++] = criar_pixel(cor, new_Vector2(size.x - 1, y));
    }

    return criar_objeto_custom(info, qtd_pixel, true);
}

Objeto* recortar_objeto(Objeto* obj, Vector2 inicio, Vector2 fim)
{
    Vector2 min_v = inicio;
    Vector2 max_v = fim;

    min_v = new_Vector2(min(inicio.x, fim.x), min(inicio.y, fim.y));
    max_v = new_Vector2(max(inicio.x, fim.x), max(inicio.y, fim.y));

    inicio = min_v;
    fim = max_v;

    int nova_qtd_pixel = 0;
    Pixel* novos_pixeis = (Pixel*)malloc(obj->qtd_pixel * sizeof(Pixel));
    for (int i = 0; i < obj->qtd_pixel; i++)
    {
        Vector2 Pixel_pos = obj->info[i].position;
        if (Pixel_pos.x >= inicio.x && Pixel_pos.x <= fim.x && Pixel_pos.y >= inicio.y && Pixel_pos.y <= fim.y)
            novos_pixeis[nova_qtd_pixel++] = obj->info[i];
    }
    novos_pixeis = realloc(novos_pixeis, nova_qtd_pixel*sizeof(Pixel));
    return criar_objeto_custom(novos_pixeis, nova_qtd_pixel, true);
}

Objeto* criar_obj_eixos_debug(Screen* s)
{
    int w = s->screen_size.x;
    int h = s->screen_size.y;
    int total = w + h - 1;
    
    Pixel* info = malloc(total * sizeof(*info));
    if (!info) { perror("malloc"); exit(1); }

    int idx = 0;
    int halfW = w/2;
    for (int dx = -halfW; dx < w - halfW; dx++) 
        info[idx++] = criar_pixel(COLOR_BRANCO, new_Vector2(dx, 0));
    
    int halfH = h/2;
    for (int dy = -halfH; dy < h - halfH; dy++)
        if (dy != 0)
            info[idx++] = criar_pixel(COLOR_BRANCO, new_Vector2(0, dy));
    
    if (idx != total) {
        fprintf(stderr, "Debug: esperado %d pixels mas gerou %d\n",
                total, idx);
        exit(1);
    }

    return criar_objeto_custom(info, total, false);
}

Objeto* merge_objeto(Objeto* prioridade, Objeto* novo, Vector2 pivot)
{
    int max_size = prioridade->qtd_pixel + novo->qtd_pixel;
    Pixel* merged_info = (Pixel*)malloc(max_size * sizeof(Pixel));
    if (!merged_info) {
        perror("Falha ao alocar memória em merge_objeto");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    for (int i = 0; i < prioridade->qtd_pixel; i++) 
        merged_info[count++] = prioridade->info[i];
    
    for (int i = 0; i < novo->qtd_pixel; i++) 
    {
        Vector2 pos_final = vector_sum(pivot, novo->info[i].position);

        bool existe = false;
        for (int j = 0; j < count; j++) 
        {
            if (compare_vector(merged_info[j].position, pos_final)) 
            {
                existe = true;
                break;
            }
        }

        if (!existe) 
            merged_info[count++] = criar_pixel(novo->info[i].cor, pos_final);
    }

    excluir_objeto(prioridade);
    excluir_objeto(novo);

    return criar_objeto_custom(merged_info, count, false);
}

Objeto* clonar_objeto(const Objeto* original) 
{
    if (!original) return NULL;

    Pixel* copia_pixels = malloc(original->qtd_pixel * sizeof(Pixel));
    if (!copia_pixels) return NULL;

    memcpy(copia_pixels, original->info, original->qtd_pixel * sizeof(Pixel));

    Objeto* novo = criar_objeto_custom(copia_pixels, original->qtd_pixel, true);

    novo->position = original->position;
    novo->renderizado = false;

    return novo;
}


Pixel deletar_pixel (Screen* s, Vector2 pos) 
{
    if (!vetor_valido_na_tela(s, pos)) 
        return criar_pixel(COLOR_PRETO, VETOR_NULO);

    if (s->pixeis[pos.y][pos.x] == NULL || s->pixeis[pos.y][pos.x]->topo == NULL)
        return criar_pixel(COLOR_PRETO, VETOR_NULO);

    if (s->pixeis[pos.y][pos.x]->topo->anterior == NULL)
        return s->pixeis[pos.y][pos.x]->topo->pixel;

    return desempilhar_pixel(s->pixeis[pos.y][pos.x]);
}

Screen* criar_tela (Vector2 tamanho, Color fundo, int limiar_de_cor)
{
    Screen* s = (Screen*)malloc(sizeof(Screen));
    s->screen_size = tamanho;
    s->position = VETOR_NULO;
    s->pixeis = (Pixel_Stack***)malloc(tamanho.y * sizeof(Pixel_Stack**));
    s->buffer = (Color**)malloc(tamanho.y * sizeof(Color*));
    s->limiar_de_cor = limiar_de_cor;
    s->obj_list = criar_lista_objetos();
    for (int i = 0; i < tamanho.y; i++)
    {
        s->pixeis[i] = (Pixel_Stack**)malloc(tamanho.x * sizeof(Pixel_Stack*));
        s->buffer[i] = (Color*)malloc(tamanho.x * sizeof(Color));
        for (int j = 0; j < tamanho.x; j++)
        {
            s->pixeis[i][j] = criar_pilha();
            s->buffer[i][j] = COR_NULA;
        }
    }
    preencher_background(s, fundo);
    return s;
}

void excluir_tela(Screen* s) 
{
    system("cls");
    if (s == NULL) return;

    if (s->pixeis != NULL) 
    {
        for (int i = 0; i < s->screen_size.y; i++) 
        {
            if (s->pixeis[i] == NULL)  continue;
        
            for (int j = 0; j < s->screen_size.x; j++) 
            {
                if (s->pixeis[i][j] == NULL) continue;

                Pixel_Stack* ps = s->pixeis[i][j];
                Pixel_Node* atual = ps->topo;
                
                while (atual != NULL) 
                {
                    Pixel_Node* anterior = atual->anterior;
                    free(atual);
                    atual = anterior;
                }
                
                free(ps);
                s->pixeis[i][j] = NULL;
            }
            free(s->pixeis[i]);
            s->pixeis[i] = NULL;
        }
        free(s->pixeis); 
        s->pixeis = NULL;
    }

    free(s); 
}

Animation criar_anim(int* frame_list, int qtd_frames, char nome[])
{
    Animation anim;
    strcpy(anim.nome, nome);
    anim.frame_atual = 0;
    anim.qtd_frames = qtd_frames;
    if (qtd_frames > 0 && frame_list != NULL)
    {
        int* idx_copy = (int*)malloc(qtd_frames * sizeof(int));
        if (idx_copy == NULL)
        {
            perror("Falha ao alocar frame_index em criar_anim");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < qtd_frames; i++)
            idx_copy[i] = frame_list[i];
        anim.frame_index = idx_copy;
    }
    else
    {
        anim.frame_index = NULL;
    }
    return anim;
}

int* obj_complexo_contem_pixel_em(ObjetoComplexo* obj, Vector2 pos, int* out_qtd_de_frames) 
{
    int* frames_que_contem = (int*)malloc(obj->qtd_frames * sizeof(int));
    int qtd_frames = 0;

    for (int i = 0; i < obj->qtd_frames; i++) 
        if (obj_contem_pixel_em(obj->frames[i], pos)) 
            frames_que_contem[qtd_frames++] = i;

    *out_qtd_de_frames = qtd_frames;

    if (qtd_frames == 0)
    {
        free(frames_que_contem);
        return NULL;
    }

    return (int*)realloc(frames_que_contem, qtd_frames * sizeof(int));
}

bool vetor_aponta_para_area_visivel(Screen* s, Vector2 vet, Vector2* out_pos_rel)
{
    Vector2 rel = vector_sum(vector_subtr(vet, s->position), centro_da_tela(s));
    if (out_pos_rel != NULL)
        *out_pos_rel = rel;
    return vetor_valido_na_tela(s, rel);
}

bool vetor_valido_na_tela(Screen *s, Vector2 vet)
{
    if (vet.x < 0 || vet.x >= s->screen_size.x || 
        vet.y < 0 || vet.y >= s->screen_size.y)
        return false;
    return true;
}

bool compare_vector (Vector2 v1, Vector2 v2)
{
    bool x = v1.x == v2.x;
    bool y = v1.y == v2.y;
    return x && y;
}

int compare_color(Color c1, Color c2)
{
    if (color_equals(c1, COR_NULA) || color_equals(c2, COR_NULA)) return INT32_MAX;
    int dist_r = (c1.r - c2.r) * (c1.r - c2.r);
    int dist_g = (c1.g - c2.g) * (c1.g - c2.g);
    int dist_b = (c1.b - c2.b) * (c1.b - c2.b);
    return dist_r + dist_g + dist_b;
}

bool obj_contem_pixel_em (Objeto* obj, Vector2 pos)
{
    for (int i = 0; i < obj->qtd_pixel; i++)
        if (compare_vector(pos, obj->info[i].position))
            return true;
    
    return false;
}

Pixel get_pixel_em(Screen* s, Vector2 pos)
{
    Vector2 rel_pos = vector_subtr(vector_sum(pos, centro_da_tela(s)), s->position);
    if (vetor_valido_na_tela(s, rel_pos))
        return s->pixeis[rel_pos.y][rel_pos.x]->topo->pixel;
    else
    {
        moveCursor(new_Vector2(0, s->screen_size.y + 3));
        printf("Tentou obter pixel na posicao {%d, %d}, que está fora dos limites\n", rel_pos.x, rel_pos.y);
        return (Pixel){COLOR_PRETO, VETOR_NULO};
    }
}

/**
 * @internal
 */
AnimationManager* criar_anim_manager(int qtd_anims)
{
    AnimationManager* anm = (AnimationManager*)malloc(sizeof(AnimationManager));
    anm->anim_atual = 0;
    anm->qtd_anims = qtd_anims;
    return anm;
}

void setar_animation_via_index(ObjetoComplexo* obj, int anim_index)
{
    if (anim_index < 0 || anim_index >= obj->anim_manager->qtd_anims) return;    
    obj->anim_manager->anim_atual = anim_index;
}

void setar_animation_via_nome(ObjetoComplexo* obj, char nome[])
{
    AnimationManager* manager = obj->anim_manager;
    for (int i = 0; i < manager->qtd_anims; i++)
    {
        if (strcmp(nome, manager->anims[i].nome) == 0)
            return setar_animation_via_index(obj, i);
    }
}

void setar_frame_obj_complexo(Screen* s, ObjetoComplexo* obj, int frame)
{
    if (obj->animar) return;
    if (frame < 0 || frame >= obj->qtd_frames) return;
    bool redesenhar = false;
    if (obj->renderizado)
    {
        redesenhar = true;
        esconder_objeto_complexo(s, obj);
    }
    obj->frame_atual = frame;
    if (redesenhar)
        desenhar_objeto_complexo(s, obj);
}

void animar_objeto_complexo(Screen* s, ObjetoComplexo* obj)
{
    if (!obj->animar) return;
    Animation* anim_atual = &(obj->anim_manager->anims[obj->anim_manager->anim_atual]);
    anim_atual->frame_atual = next_anim(anim_atual);
    bool redesenhar = false;
    if (obj->renderizado)
    {
        esconder_objeto_complexo(s, obj);
        redesenhar = true;
    }
    obj->frame_atual = anim_atual->frame_index[anim_atual->frame_atual];
    if (redesenhar)
        desenhar_objeto_complexo(s, obj);
}

void setup_animations(ObjetoComplexo* obj, Animation anims[], int qtd_anims)
{
    if (anims == NULL || obj == NULL || qtd_anims == 0) return;
    obj->anim_manager = criar_anim_manager(qtd_anims);
    Animation* copy = (Animation*)malloc(qtd_anims * sizeof(Animation));
    if (copy == NULL)
    {
        perror("Falha ao alocar anim_manager->anims");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < qtd_anims; i++)
        copy[i] = anims[i];
    obj->anim_manager->anims = copy;
}

void alterar_pivot_obj(Objeto* obj, Vector2 new_pivot)
{
    for (int i = 0; i < obj->qtd_pixel; i++)
        obj->info[i].position = vector_subtr(obj->info[i].position, new_pivot);
    
    Vector2 min = obj->info[0].position;
    Vector2 max = obj->info[0].position;

    for (int i = 1; i < obj->qtd_pixel; i++) 
    {
        if (obj->info[i].position.x < min.x) min.x = obj->info[i].position.x;
        if (obj->info[i].position.y < min.y) min.y = obj->info[i].position.y;
        if (obj->info[i].position.x > max.x) max.x = obj->info[i].position.x;
        if (obj->info[i].position.y > max.y) max.y = obj->info[i].position.y;
    }

    obj->size = new_Vector2(max.x - min.x, max.y - min.y);
}

void alterar_pivot_frame(ObjetoComplexo* obj, int frame, Vector2 novo_pivot)
{
    obj->pivot_frames[frame] = novo_pivot;
}

void desenhar_objeto(Screen* s, Objeto* obj)
{
    if (obj->renderizado) return;

    char desenhou = 0;

    for (int i = 0; i < obj->qtd_pixel; i++)
    {
        Vector2 abs_px = get_abs_pixel_pos(obj, i);
        Vector2 pixel_pos;
        if (vetor_aponta_para_area_visivel(s, abs_px, &pixel_pos))
        {
            desenhou = 1;
            add_pixel(s->pixeis[pixel_pos.y][pixel_pos.x], obj->info[i]);
        }
    }

    if (desenhou) 
    {
        obj->renderizado = true;
        if (obj->ref_node == NULL)
            incluir_na_lista(s->obj_list, (void*)obj, 'o');
    }
}

void aplicar_filtro_obj(Objeto *obj, Color filtro)
{
    for (int i = 0; i < obj->qtd_pixel; i++)
        obj->info[i].cor = aplicar_filtro(obj->info[i].cor, filtro);
}

void somar_cor_obj(Objeto* obj, Color soma)
{
    for (int i  =0; i < obj->qtd_pixel; i++)
        obj->info[i].cor = somar_cor(obj->info[i].cor, soma);
}

void espelhar_objeto(Screen *s, Objeto *obj, bool horizontalmente)
{
    bool redesenhar = false;
    if (s != NULL && obj->renderizado)
    {
        esconder_objeto(s, obj);
        redesenhar = true;
    }
    for (int i = 0; i < obj->qtd_pixel; i++)
    {
        if (horizontalmente)
            obj->info[i].position.x = -obj->info[i].position.x;
        else
            obj->info[i].position.y = -obj->info[i].position.y;
    }
    if (s != NULL && redesenhar)
        desenhar_objeto(s, obj);
}

void espelhar_objeto_complexo (Screen* s, ObjetoComplexo* obj, bool horizontalmente)
{
    bool redesenhar = false;

    if (obj->renderizado)
    {
        esconder_objeto_complexo(s, obj);
        redesenhar = true;
    }
    for (int i = 0; i < obj->qtd_frames; i++)
    {
        espelhar_objeto(s, obj->frames[i], horizontalmente);
        if (horizontalmente)
            obj->pivot_frames[i].x = -obj->pivot_frames[i].x;
        else
            obj->pivot_frames[i].y = -obj->pivot_frames[i].y;
    }

    if (redesenhar)
        desenhar_objeto_complexo(s, obj);
}

void desenhar_objeto_complexo (Screen* s, ObjetoComplexo* obj)
{
    if (obj->renderizado) return;
    int frame = obj->frame_atual;
    
    char desenhou = '0';
    for (int i = 0; i < obj->frames[frame]->qtd_pixel; i++)
    {
        Vector2 abs_px = get_complexo_abs_pixel_pos(obj, frame, i);
        Vector2 pixel_pos;
        if (vetor_aponta_para_area_visivel(s, abs_px, &pixel_pos))
        {
            desenhou = '1';
            add_pixel(s->pixeis[pixel_pos.y][pixel_pos.x], obj->frames[frame]->info[i]);
        }
    }

    if (desenhou == '1') 
    {
        obj->renderizado = true;
        if (obj->ref_node == NULL)
            incluir_na_lista(s->obj_list, (void*)obj, 'c');
    }
}

static void excluir_anim_manager(AnimationManager* manager)
{
    if (manager == NULL) return;
    for (int i = 0; i < manager->qtd_anims; i++)
        free(manager->anims[i].frame_index);
    free(manager->anims);
    free(manager);
}

void excluir_objeto (Objeto* obj)
{
    free(obj->info);
    free(obj);
}

void excluir_objeto_complexo(ObjetoComplexo* obj)
{
    if (obj == NULL) return;
    for (int i = 0; i < obj->qtd_frames; i++)
        excluir_objeto(obj->frames[i]);
    excluir_anim_manager(obj->anim_manager);
    free(obj->pivot_frames);
    free(obj->frames);
    free(obj);
}

void esconder_objeto(Screen* s, Objeto* obj)
{
    if (!obj->renderizado) return;
    
    for (int i = 0; i < obj->qtd_pixel; i++)
    {
        Vector2 abs_px = get_abs_pixel_pos(obj, i);
        Vector2 pixel_pos;
        if (vetor_aponta_para_area_visivel(s, abs_px, &pixel_pos))
            deletar_pixel(s, pixel_pos);
    }
    obj->renderizado = false;
    if (obj->ref_node != NULL)
    {
        remover_da_lista(s->obj_list, obj->ref_node);
        obj->ref_node = NULL;
    }
}

void esconder_objeto_complexo (Screen* s, ObjetoComplexo* obj)
{
    if (obj->renderizado == false) return;

    int frame = obj->frame_atual;
    for (int i = 0; i < obj->frames[obj->frame_atual]->qtd_pixel; i++)
    {
        Vector2 abs_px = get_complexo_abs_pixel_pos(obj, frame, i);
        Vector2 pixel_pos;
        if (vetor_aponta_para_area_visivel(s, abs_px, &pixel_pos))
            deletar_pixel(s, pixel_pos);
    }
    obj->renderizado = false;
    if (obj->ref_node != NULL)
    {
        remover_da_lista(s->obj_list, obj->ref_node);
        obj->ref_node = NULL;
    }
}

void mover_objeto (Screen* s, Objeto* obj, Vector2 direction)
{
    esconder_objeto(s, obj);
    obj->position = vector_sum(obj->position, direction);
    desenhar_objeto(s, obj);
}

void teleportar_objeto(Screen* s, Objeto* obj, Vector2 pos_final)
{
    esconder_objeto(s, obj);
    obj->position = pos_final;
    desenhar_objeto(s, obj);
}

void teleportar_objeto_complexo(Screen* s, ObjetoComplexo* obj, Vector2 pos_final)
{
    esconder_objeto_complexo(s, obj);
    obj->position = pos_final;
    desenhar_objeto_complexo(s, obj);
}

void preencher_background (Screen* s, Color cor)
{
    for (int i = 0; i < s->screen_size.y; i++)
        for (int j = 0; j < s->screen_size.x; j++)
            add_pixel(s->pixeis[i][j], criar_pixel(cor, new_Vector2(i, j)));
}

void print_rgb_txt(Screen* s, Color cor_do_texto, Vector2 pos, const char *format, ...) 
{
    if (!compare_vector(pos, new_Vector2(-1, -1)))
        moveCursor(pos);
    if (s == NULL)
        printf("\033[38;2;%d;%d;%dm", cor_do_texto.r, cor_do_texto.g, cor_do_texto.b);
    else
    {
        Color fundo = s->buffer[pos.y][pos.x];
        printf("\033[38;2;%d;%d;%dm", cor_do_texto.r, cor_do_texto.g, cor_do_texto.b);
        printf("\033[48;2;%d;%d;%dm", fundo.r, fundo.g, fundo.b);
    }
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\033[0m");
    printf("\033[?25l");
}

void print_rgb_txt_bg(Color cor_do_texto, Color cor_do_fundo, Vector2 pos, const char *format, ...)
{
    if (!compare_vector(pos, new_Vector2(-1, -1)))
        moveCursor(pos);

    printf("\033[38;2;%d;%d;%dm", cor_do_texto.r, cor_do_texto.g, cor_do_texto.b);
    printf("\033[48;2;%d;%d;%dm", cor_do_fundo.r, cor_do_fundo.g, cor_do_fundo.b);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\033[0m");
    printf("\033[?25l");
}

void printPixel(Pixel_Stack* p, int limiar_de_cor, Color cor_anterior)
{
    if (p == NULL || p->topo == NULL) return;
    Color c = p->topo->pixel.cor;

    if (compare_color(c, cor_anterior) > limiar_de_cor) 
        printf("\033[48;2;%d;%d;%dm", c.r, c.g, c.b);
    
    printf(" ");
}

void mover_objeto_complexo(Screen* s, ObjetoComplexo* obj, Vector2 direction)
{
    esconder_objeto_complexo(s, obj);
    obj->position = vector_sum(obj->position, direction);
    desenhar_objeto_complexo(s, obj);
}

// Mds essa foi uma das funções mais cabulosas de fazer
bool rotacionar_objeto(Screen* s, Objeto* obj, Vector2 pivot, float graus)
{
    int min_x = INT_MAX, min_y = INT_MAX;
    for (int i = 0; i < obj->qtd_pixel; i++) 
    {
        if (obj->info[i].position.x < min_x) min_x = obj->info[i].position.x;
        if (obj->info[i].position.y < min_y) min_y = obj->info[i].position.y;
    }
    Vector2 ajuste_padrao = new_Vector2(-min_x, -min_y);

    Pixel* temp = (Pixel*)malloc(obj->qtd_pixel * sizeof(Pixel));
    for (int i = 0; i < obj->qtd_pixel; i++)
        temp[i] = criar_pixel(obj->info[i].cor, vector_sum(obj->info[i].position, ajuste_padrao));
    
    pivot = vector_sum(pivot, ajuste_padrao);

    int min_rx = INT_MAX, min_ry = INT_MAX;
    int max_rx = INT_MIN, max_ry = INT_MIN;
    for (int i = 0; i < obj->qtd_pixel; i++) 
    {
        Vector2 nova_pos = aplicar_matriz_rot(temp[i].position, pivot, graus);
        if (nova_pos.x < min_rx) min_rx = nova_pos.x;
        if (nova_pos.y < min_ry) min_ry = nova_pos.y;
        if (nova_pos.x > max_rx) max_rx = nova_pos.x;
        if (nova_pos.y > max_ry) max_ry = nova_pos.y;
    }

    Vector2 ajuste_rot = new_Vector2(-min_rx, -min_ry);

    Vector2 grid_tam = new_Vector2(max_rx - min_rx + 1, max_ry - min_ry + 1);
    bool** grid = (bool**)malloc(grid_tam.x * sizeof(bool*));
    for (int i = 0; i < grid_tam.x; i++)
        grid[i] = calloc(grid_tam.y, sizeof(bool));

    Pixel* novos_pixeis = (Pixel*)malloc(obj->qtd_pixel * sizeof(Pixel));
    int novo_count = 0;

    char houve_rot = 0;
    for (int i = 0; i < obj->qtd_pixel; i++) 
    {
        Vector2 nova_pos = aplicar_matriz_rot(temp[i].position, pivot, graus);
        Vector2 ajustada = vector_sum(nova_pos, ajuste_rot);

        if (!houve_rot && (nova_pos.x != temp[i].position.x || nova_pos.y != temp[i].position.y))
            houve_rot = 1;

        if (!grid[ajustada.x][ajustada.y]) 
        {
            grid[ajustada.x][ajustada.y] = true;
            Vector2 pos_final = vector_subtr(nova_pos, ajuste_padrao);
            novos_pixeis[novo_count++] = criar_pixel(temp[i].cor, pos_final);
        }
    }

    for (int x = 0; x < grid_tam.x; x++) free(grid[x]);
    free(grid);
    free(temp);

    if (houve_rot == 0)
    {
        free(novos_pixeis);
        return false;
    }
    else
    {
        if (s != NULL)
            esconder_objeto(s, obj);
        free(obj->info);
        obj->info = realloc(novos_pixeis, novo_count * sizeof(Pixel));
        obj->qtd_pixel = novo_count;
        if (s != NULL)
            desenhar_objeto(s, obj);
        return true;
    }
}

void rotacionar_objeto_complexo (Screen* s, ObjetoComplexo* obj, Vector2 pivot, float graus)
{
    char redraw = false;
    if (s != NULL) esconder_objeto_complexo(s, obj);
    for (int i = 0; i < obj->qtd_frames; i++)
        redraw = rotacionar_objeto(NULL, obj->frames[i], pivot, graus);
    if (redraw == true) desenhar_objeto_complexo(s, obj);
}

void moveCursor(Vector2 v)
{
    printf("\033[%d;%dH", v.y + 1, v.x + 1);
}

void limpar_buffer(Screen* s) 
{
    for (int i = 0; i < s->screen_size.y; i++) 
        for (int j = 0; j < s->screen_size.x; j++) 
            s->buffer[i][j] = COR_NULA;
}

void render(Screen* s, bool reset) 
{
    if (reset)
        moveCursor(VETOR_NULO);
    printf("\033[?25l");
    printf("\033[0m\n");
    
    Color cor_anterior = COR_NULA;

    for (int i = 0; i < s->screen_size.y; i++) 
    {
        bool linha_igual = true;
        for (int j = 0; j < s->screen_size.x; j++) 
        {
            Color nova_cor = (s->pixeis[i][j] != NULL && s->pixeis[i][j]->topo != NULL)
                ? s->pixeis[i][j]->topo->pixel.cor
                : COLOR_PRETO;
            if (compare_color(s->buffer[i][j], nova_cor) > s->limiar_de_cor)
            {
                linha_igual = false;
                break;
            }
        }

        if (linha_igual && i != s->screen_size.y - 1)
        {
            moveCursor((Vector2){0, i + 1});
            continue;
        }

        moveCursor((Vector2){0, i});
        for (int j = 0; j < s->screen_size.x; j++)
        {
            if (s->pixeis[i][j] != NULL && s->pixeis[i][j]->topo != NULL)
            {
                s->buffer[i][j] = s->pixeis[i][j]->topo->pixel.cor;
                printPixel(s->pixeis[i][j], s->limiar_de_cor, cor_anterior);
            }
            else
            {
                s->buffer[i][j] = COLOR_PRETO;
                printf(" ");
            }
            cor_anterior = s->buffer[i][j];
        }
    }
    printf("\033[0m\n");
    for (int i = 0; i < s->screen_size.x; i++)
        printf("-");
    printf("\n");
    printf("\033[?25h");
}

void aplicar_filtro_obj_complexo (ObjetoComplexo* obj, Color filtro)
{
    for (int i = 0; i < obj->qtd_frames; i++)
        aplicar_filtro_obj(obj->frames[i], filtro);
}

//INTERNA
Pixel* converter_piskel_frame_para_pixel_info(const uint32_t frame_data[], int width, int height, int* qtd_pixel, Vector2 offset) 
{
    int total_pixels = width * height;
    int pixeis_coloridos = 0;

    for (int i = 0; i < total_pixels; i++)
        if(frame_data[i] != 0x00000000) 
            pixeis_coloridos++;

    Pixel* info = malloc(pixeis_coloridos * sizeof(Pixel));
    if (info == NULL) 
    {
        perror("Falha ao alocar memória para piskel frame");
        exit(EXIT_FAILURE);
    }

    int idx = 0;
    for (int i = 0; i < total_pixels; i++) 
    {
        if(frame_data[i] == 0x00000000)
            continue;
        int x = i % width;
        int y = i / width;
        Color cor = converter_ABGR_para_Color(frame_data[i]);
        info[idx++] = criar_pixel(cor, vector_sum(offset, new_Vector2(x, y)));
    }
    
    *qtd_pixel = pixeis_coloridos;
    return info;
}

Objeto* criar_piskel_obj(const uint32_t frame_data[], int width, int height)
{
    int size;
    Pixel* info = converter_piskel_frame_para_pixel_info(frame_data, width, height, &size, VETOR_NULO);
    return criar_objeto_custom(info, size, true);
}

ObjetoComplexo* criar_objeto_complexo_piskel(int qtd_frames, int width, int height, const uint32_t obj_data[qtd_frames][width * height])
{
    Objeto** obj_set = (Objeto**)malloc(qtd_frames * sizeof(Objeto*));
    for (int i = 0; i < qtd_frames; i++)
        obj_set[i] = criar_piskel_obj(obj_data[i], width, height);
    return criar_objeto_complexo_via_lista(obj_set, qtd_frames);
}

ObjetoComplexo* criar_objeto_complexo_via_lista(Objeto** obj_origem, int qtd_objetos)
{
    ObjetoComplexo* obj = (ObjetoComplexo*)malloc(sizeof(ObjetoComplexo));
    obj->pivot_frames = (Vector2*)malloc(qtd_objetos * sizeof(Vector2));
    obj->position = VETOR_NULO;
    obj->frame_atual = 0;
    obj->renderizado = false;
    obj->qtd_frames = qtd_objetos;
    obj->animar = true;
    obj->anim_manager = NULL;
    obj->ref_node = NULL;
    for (int i = 0; i < qtd_objetos; i++)
    {
        obj_origem[i]->position = VETOR_NULO;
        obj_origem[i]->renderizado = false;
        normalizar_objeto(obj_origem[i]);
    }
    obj->frames = obj_origem;
    Vector2 maior = VETOR_NULO;
    for (int i = 0; i < qtd_objetos; i++)
    {
        obj->pivot_frames[i] = VETOR_NULO;
        if (obj->frames[i]->size.x > maior.x)
            maior.x = obj->frames[i]->size.x;
        if (obj->frames[i]->size.y > maior.y)
            maior.y = obj->frames[i]->size.y;
    }
    obj->size = maior;
    
    return obj;
}

static int get_char_index(char c)
{
    c = toupper(c);

    if (c >= 'A' && c <= 'Z')
        return c - 'A';

    if (c >= '0' && c <= '9')
        return 26 + (c - '0');

    switch (c) 
    {
        case '.': return 36;
        case ',': return 37;
        case '!': return 38;
        case '?': return 39;
        default: return -10;
    }
}

static Pixel* pixel_info_do_caractere (char c, Vector2 offset, int* out_qtd_pixel, int tam_fonte)
{
    int char_index = c == '\0' || c == ' ' ? -1 : get_char_index(c);
    if (char_index < 0)
    {
        if (char_index == -10)
            printf("ERRO! Caractere nao suportado inserido -> %c\n", c);
        *out_qtd_pixel = 0;
        return NULL;
    }
    
    if (tam_fonte == 1)
        return converter_piskel_frame_para_pixel_info(
            txt_font_data_1[char_index], 
            TXT_1_CHAR_WIDTH, 
            TXT_1_CHAR_HEIGHT, 
            out_qtd_pixel, 
            offset
        );
    else if (tam_fonte == 2)
        return converter_piskel_frame_para_pixel_info(
            txt_font_data_2[char_index], 
            TXT_2_CHAR_WIDTH, 
            TXT_2_CHAR_HEIGHT, 
            out_qtd_pixel, 
            offset
        );
    else if (tam_fonte == 3)
        return converter_piskel_frame_para_pixel_info(
            txt_font_data_3[char_index], 
            TXT_3_CHAR_WIDTH, 
            TXT_3_CHAR_HEIGHT, 
            out_qtd_pixel, 
            offset
        );
    else
    {
        fprintf(stderr, "3 - Tentou criar fonte com tamanho invalido = %d. A fonte so pode ter tamanho 1, 2 ou 3", tam_fonte);
        return NULL;
    }
}

void trocar_cor_texto (Objeto* txt_obj, Color nova_cor)
{
    for (int i = 0; i < txt_obj->qtd_pixel; i++)
        txt_obj->info[i].cor = nova_cor;
}

Objeto* criar_objeto_de_texto (int espacamento, int tam_fonte, const char* txt_formatado, ...)
{
    char texto[200];
    va_list args;
    va_start(args, txt_formatado);
    vsnprintf(texto, sizeof(texto), txt_formatado, args);
    va_end(args);
    if (tam_fonte < 1 || tam_fonte > 3)
    {
        fprintf(stderr, "1 - Tentou criar fonte com tamanho invalido = %d. A fonte so pode ter tamanho 1, 2 ou 3", tam_fonte);
        return NULL;
    }

    int w;
    switch (tam_fonte)
    {
    case 1:
        w = TXT_1_CHAR_WIDTH;
        break;
    case 2:
        w = TXT_2_CHAR_WIDTH;
        break;
    case 3:
        w = TXT_3_CHAR_WIDTH;
        break;
    default:
        fprintf(stderr, "2 - Tentou criar fonte com tamanho invalido = %d. A fonte so pode ter tamanho 1, 2 ou 3", tam_fonte);
        return NULL;
        break;
    }

    int txt_size = strlen(texto);
    Pixel** totais = (Pixel**)malloc(txt_size * sizeof(Pixel*));
    int total_pixel = 0;
    int qtd_parcial[txt_size];
    Vector2 offset_atual = VETOR_NULO;
    for (int i = 0; i < txt_size; i++)
    {
        totais[i] = pixel_info_do_caractere(texto[i], offset_atual, &qtd_parcial[i], tam_fonte);
        if (qtd_parcial[i] == 0)
            offset_atual.x += (w + espacamento) / 2;
        else
            offset_atual.x += w + espacamento;
        total_pixel += qtd_parcial[i];
    }
    Pixel* info = (Pixel*)malloc(total_pixel * sizeof(Pixel));
    int i = -1;
    for (int j = 0; j < txt_size; j++)
    {
        if (totais[j] != NULL)
        {
            for (int k = 0; k < qtd_parcial[j]; k++)
            {
                info[++i] = totais[j][k];
            }
        }
    }
    for (int j = 0; j < txt_size; j++) if (totais[j] != NULL)
        free(totais[j]);
    
    free(totais);
    return criar_objeto_custom(info, total_pixel, true);
}
