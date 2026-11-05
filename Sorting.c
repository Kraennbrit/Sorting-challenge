/*
 * Sorting.c - Desafio de ordenação (menor tempo de execução)
 *
 * Estratégia: Counting Sort, com Radix Sort (LSD) como plano B quando o
 * intervalo de valores é grande demais para o vetor de contagem.
 *
 * Fluxo do programa:
 *   1. Lê todos os inteiros do arquivo de entrada (fora do cronômetro).
 *   2. Ordena o vetor em memória (única parte cronometrada, com clock()).
 *   3. Verifica se o resultado está em ordem crescente (fora do cronômetro).
 *   4. Grava o vetor ordenado no arquivo de saída (fora do cronômetro).
 *
 * Compilação:  gcc -O2 -o counting_sort Sorting.c
 * Execução:    ./counting_sort [arquivo_entrada] [arquivo_saida]
 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

/* Nomes de arquivo usados quando nenhum argumento é informado. */
#define ARQUIVO_ENTRADA_PADRAO "listaOdenada.txt"
#define ARQUIVO_SAIDA_PADRAO   "listaOrdenada_saida.txt"

/* Capacidade inicial do vetor de números; dobra quando enche. */
#define CAPACIDADE_INICIAL 1024

/* O Counting Sort só é usado se o intervalo (max - min + 1) for no máximo
 * FATOR_INTERVALO vezes o número de elementos. Acima disso, o vetor de
 * contagem fica grande e lento demais, e o Radix Sort passa a compensar. */
#define FATOR_INTERVALO 8

/* ------------------------------------------------------------------ */
/* Leitura                                                            */
/* ------------------------------------------------------------------ */

/* Lê todos os inteiros do arquivo 'caminho' (qualquer separador não numérico).
 * Devolve um vetor alocado com malloc (quem chama deve dar free) e grava a
 * quantidade de números em *n. Devolve NULL em caso de erro. */
static int *ler_lista(const char *caminho, size_t *n)
{
    FILE *f = fopen(caminho, "rb");
    if (!f) {
        perror(caminho);
        return NULL;
    }

    /* Descobre o tamanho do arquivo para lê-lo de uma só vez (bem mais
     * rápido do que chamar fscanf milhões de vezes). */
    if (fseek(f, 0, SEEK_END) != 0) { perror(caminho); fclose(f); return NULL; }
    long tamanho = ftell(f);
    if (tamanho < 0)                { perror(caminho); fclose(f); return NULL; }
    rewind(f);

    char *texto = malloc((size_t)tamanho + 1);
    if (!texto) {
        fprintf(stderr, "Sem memoria para ler o arquivo.\n");
        fclose(f);
        return NULL;
    }
    if (fread(texto, 1, (size_t)tamanho, f) != (size_t)tamanho) {
        fprintf(stderr, "Erro ao ler %s.\n", caminho);
        free(texto);
        fclose(f);
        return NULL;
    }
    fclose(f);
    texto[tamanho] = '\0';

    size_t capacidade = CAPACIDADE_INICIAL, qtd = 0;
    int *numeros = malloc(capacidade * sizeof *numeros);
    if (!numeros) {
        fprintf(stderr, "Sem memoria para o vetor.\n");
        free(texto);
        return NULL;
    }

    const char *p = texto;
    while (*p) {
        /* Pula tudo que não é início de número (espaços, quebras de linha...). */
        while (*p && *p != '-' && (*p < '0' || *p > '9')) p++;
        if (!*p) break;

        int negativo = 0;
        if (*p == '-') { negativo = 1; p++; }

        /* Converte os dígitos. Usa long long e confere o limite de int para
         * não corromper valores silenciosamente. */
        long long valor = 0;
        while (*p >= '0' && *p <= '9') {
            valor = valor * 10 + (*p++ - '0');
            if (valor > (long long)INT_MAX + 1) {
                fprintf(stderr, "Valor fora do intervalo de int no arquivo.\n");
                free(numeros);
                free(texto);
                return NULL;
            }
        }
        if (negativo) valor = -valor;
        if (valor > INT_MAX || valor < INT_MIN) {
            fprintf(stderr, "Valor fora do intervalo de int no arquivo.\n");
            free(numeros);
            free(texto);
            return NULL;
        }

        /* Cresce o vetor dobrando a capacidade quando necessário. */
        if (qtd == capacidade) {
            capacidade *= 2;
            int *novo = realloc(numeros, capacidade * sizeof *numeros);
            if (!novo) {
                fprintf(stderr, "Sem memoria para o vetor.\n");
                free(numeros);
                free(texto);
                return NULL;
            }
            numeros = novo;
        }
        numeros[qtd++] = (int)valor;
    }

    free(texto);
    *n = qtd;
    return numeros;
}

/* ------------------------------------------------------------------ */
/* Algoritmos de ordenação                                            */
/* ------------------------------------------------------------------ */

/* Counting Sort: O(n + m), com m = max - min + 1.
 * Conta quantas vezes cada valor aparece e reescreve o vetor em ordem.
 * Devolve 0 em sucesso e -1 se faltar memória. */
static int counting_sort(int *a, size_t n, int min, int max)
{
    size_t m = (size_t)((long long)max - min + 1);

    /* Contadores de 32 bits ocupam metade da memória de size_t e cabem
     * melhor no cache. calloc já entrega tudo zerado. */
    uint32_t *contagem = calloc(m, sizeof *contagem);
    if (!contagem) {
        fprintf(stderr, "Sem memoria para o vetor de contagem.\n");
        return -1;
    }

    /* O valor min vira o índice 0 (isso permite números negativos). */
    for (size_t i = 0; i < n; i++)
        contagem[(size_t)((long long)a[i] - min)]++;

    /* Reescreve o vetor original, valor por valor, na ordem crescente. */
    size_t k = 0;
    for (size_t v = 0; v < m; v++) {
        int valor = (int)((long long)v + min);
        for (uint32_t c = contagem[v]; c > 0; c--)
            a[k++] = valor;
    }

    free(contagem);
    return 0;
}

/* Radix Sort LSD: 4 passadas de 8 bits, O(n). Plano B para quando o
 * intervalo de valores é grande. Inverter o bit de sinal faz os negativos
 * ficarem antes dos positivos. Devolve 0 em sucesso e -1 sem memória. */
static int radix_sort(int *a, size_t n)
{
    int *temp = malloc(n * sizeof *temp);
    if (!temp) {
        fprintf(stderr, "Sem memoria para o vetor auxiliar.\n");
        return -1;
    }

    int *origem = a, *destino = temp;

    for (int passada = 0; passada < 4; passada++) {
        int deslocamento = passada * 8;
        size_t contagem[256] = {0};

        for (size_t i = 0; i < n; i++) {
            unsigned chave = (unsigned)origem[i] ^ 0x80000000u;
            contagem[(chave >> deslocamento) & 0xFF]++;
        }

        /* Soma acumulada: posição inicial de cada "balde" no destino. */
        size_t posicao = 0;
        for (int b = 0; b < 256; b++) {
            size_t c = contagem[b];
            contagem[b] = posicao;
            posicao += c;
        }

        /* Distribuição estável (mantém a ordem das passadas anteriores). */
        for (size_t i = 0; i < n; i++) {
            unsigned chave = (unsigned)origem[i] ^ 0x80000000u;
            destino[contagem[(chave >> deslocamento) & 0xFF]++] = origem[i];
        }

        int *troca = origem; origem = destino; destino = troca;
    }

    /* Após 4 passadas (número par), o resultado já está de volta em 'a'. */
    free(temp);
    return 0;
}

/* Escolhe o algoritmo conforme os dados e ordena 'a' no próprio vetor.
 * Devolve 0 em sucesso e -1 em caso de erro. */
static int ordenar(int *a, size_t n)
{
    if (n < 2) return 0;

    /* Uma passada para achar o menor e o maior valor. */
    int min = a[0], max = a[0];
    for (size_t i = 1; i < n; i++) {
        if (a[i] < min) min = a[i];
        if (a[i] > max) max = a[i];
    }

    unsigned long long intervalo = (unsigned long long)((long long)max - min) + 1;

    if (n <= UINT32_MAX && intervalo <= (unsigned long long)FATOR_INTERVALO * n)
        return counting_sort(a, n, min, max);

    return radix_sort(a, n);
}

/* ------------------------------------------------------------------ */
/* Verificação e escrita                                              */
/* ------------------------------------------------------------------ */

/* Devolve 1 se o vetor está em ordem crescente, 0 caso contrário. */
static int esta_ordenado(const int *a, size_t n)
{
    for (size_t i = 1; i < n; i++)
        if (a[i - 1] > a[i]) return 0;
    return 1;
}

/* Grava um número por linha em 'caminho'. Devolve 0 em sucesso, -1 em erro. */
static int gravar_lista(const char *caminho, const int *a, size_t n)
{
    FILE *f = fopen(caminho, "w");
    if (!f) {
        perror(caminho);
        return -1;
    }
    for (size_t i = 0; i < n; i++)
        fprintf(f, "%d\n", a[i]);
    fclose(f);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Programa principal                                                 */
/* ------------------------------------------------------------------ */

int main(int argc, char **argv)
{
    const char *entrada = argc > 1 ? argv[1] : ARQUIVO_ENTRADA_PADRAO;
    const char *saida   = argc > 2 ? argv[2] : ARQUIVO_SAIDA_PADRAO;

    /* 1. Leitura (não entra na medição). */
    size_t n = 0;
    int *a = ler_lista(entrada, &n);
    if (!a) return 1;

    /* 2. Ordenação: somente este trecho é cronometrado. */
    clock_t inicio = clock();
    int erro = ordenar(a, n);
    clock_t fim = clock();

    if (erro != 0) {
        free(a);
        return 1;
    }

    double segundos = (double)(fim - inicio) / CLOCKS_PER_SEC;
    printf("Tempo de ordenacao: %f segundos (%f ms)\n", segundos, segundos * 1000.0);

    /* 3. Verificação de corretude (fora da medição). */
    if (!esta_ordenado(a, n)) {
        fprintf(stderr, "ERRO: o vetor nao ficou ordenado.\n");
        free(a);
        return 1;
    }

    /* 4. Gravação do resultado (fora da medição). */
    if (gravar_lista(saida, a, n) != 0) {
        free(a);
        return 1;
    }
    printf("%zu numeros ordenados -> %s\n", n, saida);

    free(a);
    return 0;
}
