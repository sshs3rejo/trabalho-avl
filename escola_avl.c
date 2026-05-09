#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define MAX_ESTADO 3
#define MAX_MUNICIPIO 100
#define MAX_REDE 20
#define MAX_LINHA 256
#define MAX_BATCHES 200

typedef struct {
    int id;
    char estado[MAX_ESTADO];
    char municipio[MAX_MUNICIPIO];
    char rede[MAX_REDE];
    float media_cn;
    float media_ch;
    float media_l;
    float media_m;
    float media_r;
} Escola;

typedef struct no_bst {
    Escola dado;
    struct no_bst *esq;
    struct no_bst *dir;
} NoBST;

typedef struct no_avl {
    Escola dado;
    struct no_avl *esq;
    struct no_avl *dir;
    int altura;
} NoAVL;

static void strcpy_safe(char *dest, const char *src, size_t n)
{
    strncpy(dest, src, n - 1);
    dest[n - 1] = '\0';
}

static int ler_escolas(const char *caminho, Escola **escolas, int *n)
{
    FILE *f = fopen(caminho, "r");
    if (!f) { perror("Erro abrir DadosEnem.txt"); return 0; }

    int cap = 4096;
    *escolas = malloc(cap * sizeof(Escola));
    if (!*escolas) { fclose(f); return 0; }

    *n = 0;
    char linha[MAX_LINHA];

    while (fgets(linha, sizeof(linha), f)) {
        if (*n >= cap) {
            cap *= 2;
            Escola *tmp = realloc(*escolas, cap * sizeof(Escola));
            if (!tmp) { free(*escolas); fclose(f); return 0; }
            *escolas = tmp;
        }

        Escola *e = &(*escolas)[*n];
        char *tok = strtok(linha, ";\n");
        if (!tok) continue;
        e->id = atoi(tok);

        tok = strtok(NULL, ";\n");
        if (tok) strcpy_safe(e->estado, tok, sizeof(e->estado));
        tok = strtok(NULL, ";\n");
        if (tok) strcpy_safe(e->municipio, tok, sizeof(e->municipio));
        tok = strtok(NULL, ";\n");
        if (tok) strcpy_safe(e->rede, tok, sizeof(e->rede));
        tok = strtok(NULL, ";\n");
        if (tok) e->media_cn = atof(tok);
        tok = strtok(NULL, ";\n");
        if (tok) e->media_ch = atof(tok);
        tok = strtok(NULL, ";\n");
        if (tok) e->media_l = atof(tok);
        tok = strtok(NULL, ";\n");
        if (tok) e->media_m = atof(tok);
        tok = strtok(NULL, ";\n");
        if (tok) e->media_r = atof(tok);

        (*n)++;
    }
    fclose(f);
    return 1;
}

static int ler_indices(const char *caminho, int **indices)
{
    FILE *f = fopen(caminho, "r");
    if (!f) { perror("Erro abrir indices.txt"); return 0; }

    int cap = 65536;
    int n = 0;
    *indices = malloc(cap * sizeof(int));
    if (!*indices) { fclose(f); return 0; }

    char linha[32];
    while (fgets(linha, sizeof(linha), f)) {
        if (n >= cap) {
            cap *= 2;
            int *tmp = realloc(*indices, cap * sizeof(int));
            if (!tmp) { free(*indices); fclose(f); return 0; }
            *indices = tmp;
        }
        (*indices)[n++] = atoi(linha);
    }
    fclose(f);
    return n;
}

static NoBST *bst_criar_no(Escola e)
{
    NoBST *no = malloc(sizeof(NoBST));
    no->dado = e;
    no->esq = no->dir = NULL;
    return no;
}

static NoBST *bst_inserir(NoBST *raiz, Escola e)
{
    if (!raiz) return bst_criar_no(e);
    if (e.id < raiz->dado.id)
        raiz->esq = bst_inserir(raiz->esq, e);
    else if (e.id > raiz->dado.id)
        raiz->dir = bst_inserir(raiz->dir, e);
    return raiz;
}

static int bst_buscar(NoBST *raiz, int id, Escola *ret)
{
    if (!raiz) return 0;
    if (id == raiz->dado.id) { *ret = raiz->dado; return 1; }
    return bst_buscar(id < raiz->dado.id ? raiz->esq : raiz->dir, id, ret);
}

static NoBST *bst_minimo(NoBST *raiz)
{
    while (raiz && raiz->esq) raiz = raiz->esq;
    return raiz;
}

static NoBST *bst_remover(NoBST *raiz, int id)
{
    if (!raiz) return NULL;
    if (id < raiz->dado.id)
        raiz->esq = bst_remover(raiz->esq, id);
    else if (id > raiz->dado.id)
        raiz->dir = bst_remover(raiz->dir, id);
    else {
        if (!raiz->esq) {
            NoBST *filho = raiz->dir;
            free(raiz);
            return filho;
        }
        if (!raiz->dir) {
            NoBST *filho = raiz->esq;
            free(raiz);
            return filho;
        }
        NoBST *sucessor = bst_minimo(raiz->dir);
        raiz->dado = sucessor->dado;
        raiz->dir = bst_remover(raiz->dir, sucessor->dado.id);
    }
    return raiz;
}

static int bst_altura(NoBST *raiz)
{
    if (!raiz) return -1;
    int ae = bst_altura(raiz->esq);
    int ad = bst_altura(raiz->dir);
    return 1 + (ae > ad ? ae : ad);
}

static void bst_liberar(NoBST *raiz)
{
    if (!raiz) return;
    bst_liberar(raiz->esq);
    bst_liberar(raiz->dir);
    free(raiz);
}

static int avl_alt(NoAVL *no)
{
    return no ? no->altura : -1;
}

static int avl_fb(NoAVL *no)
{
    return no ? avl_alt(no->esq) - avl_alt(no->dir) : 0;
}

static NoAVL *avl_criar_no(Escola e)
{
    NoAVL *no = malloc(sizeof(NoAVL));
    no->dado = e;
    no->esq = no->dir = NULL;
    no->altura = 0;
    return no;
}

static int max2(int a, int b) { return a > b ? a : b; }

static NoAVL *avl_rot_dir(NoAVL *y)
{
    NoAVL *x = y->esq;
    NoAVL *T2 = x->dir;
    x->dir = y;
    y->esq = T2;
    y->altura = 1 + max2(avl_alt(y->esq), avl_alt(y->dir));
    x->altura = 1 + max2(avl_alt(x->esq), avl_alt(x->dir));
    return x;
}

static NoAVL *avl_rot_esq(NoAVL *x)
{
    NoAVL *y = x->dir;
    NoAVL *T2 = y->esq;
    y->esq = x;
    x->dir = T2;
    x->altura = 1 + max2(avl_alt(x->esq), avl_alt(x->dir));
    y->altura = 1 + max2(avl_alt(y->esq), avl_alt(y->dir));
    return y;
}

static NoAVL *avl_inserir(NoAVL *raiz, Escola e)
{
    if (!raiz) return avl_criar_no(e);
    if (e.id < raiz->dado.id)
        raiz->esq = avl_inserir(raiz->esq, e);
    else if (e.id > raiz->dado.id)
        raiz->dir = avl_inserir(raiz->dir, e);
    else
        return raiz;

    raiz->altura = 1 + max2(avl_alt(raiz->esq), avl_alt(raiz->dir));
    int fb = avl_fb(raiz);

    if (fb > 1 && e.id < raiz->esq->dado.id)
        return avl_rot_dir(raiz);
    if (fb < -1 && e.id > raiz->dir->dado.id)
        return avl_rot_esq(raiz);
    if (fb > 1 && e.id > raiz->esq->dado.id) {
        raiz->esq = avl_rot_esq(raiz->esq);
        return avl_rot_dir(raiz);
    }
    if (fb < -1 && e.id < raiz->dir->dado.id) {
        raiz->dir = avl_rot_dir(raiz->dir);
        return avl_rot_esq(raiz);
    }
    return raiz;
}

static int avl_buscar(NoAVL *raiz, int id, Escola *ret)
{
    if (!raiz) return 0;
    if (id == raiz->dado.id) { *ret = raiz->dado; return 1; }
    return avl_buscar(id < raiz->dado.id ? raiz->esq : raiz->dir, id, ret);
}

static NoAVL *avl_minimo(NoAVL *raiz)
{
    while (raiz && raiz->esq) raiz = raiz->esq;
    return raiz;
}

static NoAVL *avl_remover(NoAVL *raiz, int id)
{
    if (!raiz) return NULL;
    if (id < raiz->dado.id)
        raiz->esq = avl_remover(raiz->esq, id);
    else if (id > raiz->dado.id)
        raiz->dir = avl_remover(raiz->dir, id);
    else {
        if (!raiz->esq || !raiz->dir) {
            NoAVL *filho = raiz->esq ? raiz->esq : raiz->dir;
            if (!filho) {
                free(raiz);
                return NULL;
            }
            NoAVL *temp = raiz;
            raiz = filho;
            free(temp);
        } else {
            NoAVL *sucessor = avl_minimo(raiz->dir);
            raiz->dado = sucessor->dado;
            raiz->dir = avl_remover(raiz->dir, sucessor->dado.id);
        }
    }
    if (!raiz) return NULL;

    raiz->altura = 1 + max2(avl_alt(raiz->esq), avl_alt(raiz->dir));
    int fb = avl_fb(raiz);

    if (fb > 1 && avl_fb(raiz->esq) >= 0)
        return avl_rot_dir(raiz);
    if (fb > 1 && avl_fb(raiz->esq) < 0) {
        raiz->esq = avl_rot_esq(raiz->esq);
        return avl_rot_dir(raiz);
    }
    if (fb < -1 && avl_fb(raiz->dir) <= 0)
        return avl_rot_esq(raiz);
    if (fb < -1 && avl_fb(raiz->dir) > 0) {
        raiz->dir = avl_rot_dir(raiz->dir);
        return avl_rot_esq(raiz);
    }
    return raiz;
}

static void avl_liberar(NoAVL *raiz)
{
    if (!raiz) return;
    avl_liberar(raiz->esq);
    avl_liberar(raiz->dir);
    free(raiz);
}

static double calc_media(double *v, int n)
{
    double s = 0;
    for (int i = 0; i < n; i++) s += v[i];
    return s / n;
}

static double calc_dp(double *v, int n, double med)
{
    double s = 0;
    for (int i = 0; i < n; i++) s += (v[i] - med) * (v[i] - med);
    return sqrt(s / n);
}

int main(void)
{
    Escola *escolas = NULL;
    int total_escolas = 0;
    if (!ler_escolas("DadosEnem.txt", &escolas, &total_escolas)) {
        fprintf(stderr, "Falha ao ler DadosEnem.txt\n");
        return 1;
    }
    printf("Escolas carregadas: %d\n\n", total_escolas);

    int *indices = NULL;
    int total_indices = ler_indices("indices.txt", &indices);
    if (total_indices <= 0) {
        fprintf(stderr, "Falha ao ler indices.txt\n");
        free(escolas);
        return 1;
    }
    printf("Indices carregados: %d\n\n", total_indices);

    
    clock_t t0 = clock();
    NoBST *bst = NULL;
    for (int i = 0; i < total_escolas; i++)
        bst = bst_inserir(bst, escolas[i]);
    clock_t t1 = clock();
    double t_bst_const = (double)(t1 - t0) / CLOCKS_PER_SEC;

    int h_bst = bst_altura(bst);
    int h_bst_esq = bst ? bst_altura(bst->esq) : -1;
    int h_bst_dir = bst ? bst_altura(bst->dir) : -1;

    printf("========== ARVORE BINARIA DE BUSCA ==========\n");
    printf("Tempo de construcao:     %.4f s\n", t_bst_const);
    printf("Altura:                  %d\n", h_bst);
    printf("Niveis:                  %d\n", h_bst + 1);
    printf("Altura sub-arvore esq:   %d\n", h_bst_esq);
    printf("Altura sub-arvore dir:   %d\n\n", h_bst_dir);

    
    t0 = clock();
    NoAVL *avl = NULL;
    for (int i = 0; i < total_escolas; i++)
        avl = avl_inserir(avl, escolas[i]);
    t1 = clock();
    double t_avl_const = (double)(t1 - t0) / CLOCKS_PER_SEC;

    int h_avl = avl ? avl->altura : -1;
    int h_avl_esq = avl ? avl_alt(avl->esq) : -1;
    int h_avl_dir = avl ? avl_alt(avl->dir) : -1;

    printf("========== ARVORE AVL ==========\n");
    printf("Tempo de construcao:     %.4f s\n", t_avl_const);
    printf("Altura:                  %d\n", h_avl);
    printf("Niveis:                  %d\n", h_avl + 1);
    printf("Altura sub-arvore esq:   %d\n", h_avl_esq);
    printf("Altura sub-arvore dir:   %d\n\n", h_avl_dir);

    
    Escola ret;
    volatile long long sink = 0;
    t0 = clock();
    for (int i = 0; i < total_indices; i++)
        sink += bst_buscar(bst, indices[i], &ret);
    t1 = clock();
    double t_bst_busca = (double)(t1 - t0) / CLOCKS_PER_SEC;

    t0 = clock();
    for (int i = 0; i < total_indices; i++)
        sink += avl_buscar(avl, indices[i], &ret);
    t1 = clock();
    double t_avl_busca = (double)(t1 - t0) / CLOCKS_PER_SEC;

    printf("========== BUSCA TOTAL (%d operacoes) ==========\n", total_indices);
    printf("BST: %.4f s  (%.8f s por busca)\n", t_bst_busca, t_bst_busca / total_indices);
    printf("AVL: %.4f s  (%.8f s por busca)\n\n", t_avl_busca, t_avl_busca / total_indices);

    
    printf("========== ESTATISTICA POR LOTES CRESCENTES ==========\n");

    double bst_lotes[MAX_BATCHES], avl_lotes[MAX_BATCHES];
    int tamanhos[MAX_BATCHES];
    int nlotes = 0;

    for (int s = 1000; s <= total_indices && nlotes < MAX_BATCHES; s += 1000) {
        tamanhos[nlotes] = s;

        sink = 0;
        t0 = clock();
        for (int i = 0; i < s; i++)
            sink += bst_buscar(bst, indices[i], &ret);
        t1 = clock();
        bst_lotes[nlotes] = (double)(t1 - t0) / CLOCKS_PER_SEC;

        sink = 0;
        t0 = clock();
        for (int i = 0; i < s; i++)
            sink += avl_buscar(avl, indices[i], &ret);
        t1 = clock();
        avl_lotes[nlotes] = (double)(t1 - t0) / CLOCKS_PER_SEC;

        nlotes++;
    }

    double med_bst = calc_media(bst_lotes, nlotes);
    double dp_bst  = calc_dp(bst_lotes, nlotes, med_bst);
    double med_avl = calc_media(avl_lotes, nlotes);
    double dp_avl  = calc_dp(avl_lotes, nlotes, med_avl);

    printf("BST - tempo medio: %.6f s | desvio padrao: %.6f s\n", med_bst, dp_bst);
    printf("AVL - tempo medio: %.6f s | desvio padrao: %.6f s\n\n", med_avl, dp_avl);

    
    printf("Detalhamento (primeiros 10 lotes):\n");
    for (int i = 0; i < 10 && i < nlotes; i++)
        printf("  %6d buscas -> BST: %.6f s | AVL: %.6f s\n",
               tamanhos[i], bst_lotes[i], avl_lotes[i]);
    if (nlotes > 15) {
        printf("  ...\n");
        printf("Detalhamento (ultimos 5 lotes):\n");
        for (int i = nlotes - 5; i < nlotes; i++)
            printf("  %6d buscas -> BST: %.6f s | AVL: %.6f s\n",
                   tamanhos[i], bst_lotes[i], avl_lotes[i]);
    }
    printf("\n");

    
    printf("========== INSERCAO / REMOCAO (amostra de 1000) ==========\n");

    NoBST *bst2 = NULL;
    t0 = clock();
    for (int i = 0; i < 1000; i++)
        bst2 = bst_inserir(bst2, escolas[i]);
    t1 = clock();
    double t_bst_ins = (double)(t1 - t0) / CLOCKS_PER_SEC;

    t0 = clock();
    for (int i = 0; i < 1000; i++)
        bst2 = bst_remover(bst2, escolas[i].id);
    t1 = clock();
    double t_bst_rem = (double)(t1 - t0) / CLOCKS_PER_SEC;
    bst_liberar(bst2);

    NoAVL *avl2 = NULL;
    t0 = clock();
    for (int i = 0; i < 1000; i++)
        avl2 = avl_inserir(avl2, escolas[i]);
    t1 = clock();
    double t_avl_ins = (double)(t1 - t0) / CLOCKS_PER_SEC;

    t0 = clock();
    for (int i = 0; i < 1000; i++)
        avl2 = avl_remover(avl2, escolas[i].id);
    t1 = clock();
    double t_avl_rem = (double)(t1 - t0) / CLOCKS_PER_SEC;
    avl_liberar(avl2);

    printf("BST insercao (1000):  %.6f s\n", t_bst_ins);
    printf("BST remocao  (1000):  %.6f s\n", t_bst_rem);
    printf("AVL insercao (1000):  %.6f s\n", t_avl_ins);
    printf("AVL remocao  (1000):  %.6f s\n\n", t_avl_rem);

    
    bst_liberar(bst);
    avl_liberar(avl);
    free(escolas);
    free(indices);

    return 0;
}
