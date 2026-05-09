# Roteiro de Apresentação — Árvores AVL vs BST

## Estrutura do Projeto

### Arquivos
- `DadosEnem.txt` — 30.227 escolas do ENEM (id;estado;municipio;rede;5 médias)
- `indices.txt` — 999.999 IDs para operações de busca
- `escola_avl.c` — implementação única (~430 linhas)

### Estruturas de Dados

```c
typedef struct {
    int id;
    char estado[3], municipio[100], rede[20];
    float media_cn, media_ch, media_l, media_m, media_r;
} Escola;
```

```c
// BST — sem balanceamento
typedef struct no_bst {
    Escola dado;
    struct no_bst *esq, *dir;
} NoBST;

// AVL — com fator de balanceamento
typedef struct no_avl {
    Escola dado;
    struct no_avl *esq, *dir;
    int altura;  // <-- diferenca crucial
} NoAVL;
```

## BST (Árvore Binária de Busca)

### Inserção
```
SE raiz vazia: criar no
SENAO SE id < raiz.id: inserir na esquerda
SENAO SE id > raiz.id: inserir na direita
```

### Busca por Referência
Retorna 1 se encontrou, 0 se não. O ponteiro `Escola *ret` recebe os dados.

### Remoção (3 casos)
1. Folha — remove direto
2. 1 filho — substitui pelo filho
3. 2 filhos — substitui pelo sucessor em ordem (menor da direita)

### Problema
Pode degenerar para O(n) se os dados forem inseridos em ordem crescente.

---

## AVL (Árvore Balanceada)

### Altura e Fator de Balanceamento
```c
altura(nó)   = 1 + max(altura(esq), altura(dir))
fb(nó)       = altura(esq) - altura(dir)   // deve ser -1, 0 ou +1
```

### Rotações (4 casos)

| Caso | FB | Ação |
|------|----|------|
| Esquerda-Esquerda | fb > 1, filho-esq estável | Rotação direita |
| Direita-Direita | fb < -1, filho-dir estável | Rotação esquerda |
| Esquerda-Direita | fb > 1, filho-esq desbalanceado p/ dir | Rotação dupla E->D |
| Direita-Esquerda | fb < -1, filho-dir desbalanceado p/ esq | Rotação dupla D->E |

### Rotação Direita (ilustração)
```
    y              x
   / \            / \
  x   T3   =>    T1  y
 / \                / \
T1  T2             T2  T3
```

### Inserção AVL
```
1. Inserir como BST
2. Atualizar altura do no
3. Calcular FB
4. Se FB > 1 ou FB < -1: rotacionar
```

---

## Leitura dos Arquivos

### DadosEnem.txt
Campos separados por `;`. Usa `strtok()` para quebrar linha.
```
4810;MG;ABADIA DOS DOU...;State;458.47;462.28;504.08;527.94;551.06
```

### indices.txt
Um ID por linha. 999.999 IDs.

---

## Medição de Desempenho

Usa `clock()` da biblioteca `<time.h>`:

```c
clock_t t0 = clock();
// operacao
clock_t t1 = clock();
double tempo = (double)(t1 - t0) / CLOCKS_PER_SEC;
```

### O que é medido
1. **Construção** — tempo para inserir 30.227 escolas
2. **Altura / Níveis** — estrutura das árvores
3. **Busca total** — 999.999 buscas sequenciais
4. **Lotes crescentes** — 1.000, 2.000, 3.000... até 200.000 buscas
5. **Inserção/Remoção** — 1.000 operações em árvore separada

### Prevenção de Otimização
Usa `volatile long long sink` para evitar que o compilador elimine loops de busca.

---

## Resultados Obtidos

| Métrica | BST | AVL |
|---------|-----|-----|
| Altura | 44 | 17 |
| Níveis | 45 | 18 |
| Construção | 0.0126 s | 0.0128 s |
| Busca 1M | 0.1617 s | 0.1032 s |
| Inserção 1000 | 0.000200 s | 0.000138 s |
| Remoção 1000 | 0.000086 s | 0.000116 s |

### Conclusões
- AVL tem altura **2.5x menor** que BST (17 vs 44)
- Busca **36% mais rápida** na AVL para 1M operações
- Custo de construção é quase igual (AVL 1.5% maior)
- Balanceamento vale a pena quando busca é frequente

---

## Compilação e Execução

```bash
gcc -O2 -o escola_avl escola_avl.c -lm
./escola_avl
```

A flag `-O2` otimiza o código. `-lm` linka a biblioteca math (sqrt para desvio padrão).
