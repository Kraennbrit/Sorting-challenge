#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*O código precisa ser comentado, *assim**/
//vou usar Counting sort
//compilado com gcc 

//gcc -O2 -o counting_sort Sorting.c
//./counting_sort

/* Le todos os inteiros (um por linha) do arquivo em 'caminho'.
 * Retorna um vetor alocado com malloc (o chamador faz free) e grava o tamanho em *n.
 * Retorna NULL em caso de erro. */
int *ler_lista(const char *caminho, size_t *n)
{
    FILE *f = fopen(caminho, "rb");
    if (!f) { perror(caminho); return NULL; }

    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(tam + 1);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, tam, f) != (size_t)tam) { free(buf); fclose(f); return NULL; }
    fclose(f);
    buf[tam] = '\0';

    size_t cap = 1024, cnt = 0;
    int *a = malloc(cap * sizeof *a);
    if (!a) { free(buf); return NULL; }

    char *p = buf;
    while (*p) {
        /* pula tudo que nao for inicio de numero */
        while (*p && *p != '-' && (*p < '0' || *p > '9')) p++;
        if (!*p) break;

        int neg = 0;
        if (*p == '-') { neg = 1; p++; }
        long v = 0;
        while (*p >= '0' && *p <= '9') v = v * 10 + (*p++ - '0');

        if (cnt == cap) {
            cap *= 2;
            int *tmp = realloc(a, cap * sizeof *a);
            if (!tmp) { free(a); free(buf); return NULL; }
            a = tmp;
        }
        a[cnt++] = neg ? (int)-v : (int)v;
    }
    free(buf);
    *n = cnt;
    return a;
}

/* Counting sort: O(n + m), onde m = max - min + 1. Ordena 'a' no proprio vetor. */
int counting_sort(int *a, size_t n)
{
    if (n == 0) return 0;

    int min = a[0], max = a[0];
    for (size_t i = 1; i < n; i++) {
        if (a[i] < min) min = a[i];
        if (a[i] > max) max = a[i];
    }

    size_t m = (size_t)((long long)max - min + 1);
    size_t *c = calloc(m, sizeof *c);
    if (!c) { fprintf(stderr, "sem memoria para o vetor de contagem\n"); return -1; }

    for (size_t i = 0; i < n; i++) c[(size_t)((long long)a[i] - min)]++;

    size_t k = 0;
    for (size_t v = 0; v < m; v++)
        for (size_t j = c[v]; j > 0; j--)
            a[k++] = (int)(v + min);

    free(c);
    return 0;
}

/* Le o arquivo 'entrada', ordena e grava o resultado em 'saida'. */
int ordenar_arquivo(const char *entrada, const char *saida)
{
    size_t n = 0;
    int *a = ler_lista(entrada, &n);
    if (!a) return -1;

    /* Cronometro: apenas a ordenacao (leitura e escrita ficam de fora) */
    clock_t inicio = clock();

    int erro = counting_sort(a, n);

    clock_t fim = clock();
    double tempo_gasto = ((double)(fim - inicio)) / CLOCKS_PER_SEC;

    if (erro != 0) { free(a); return -1; }

    printf("Tempo de ordenacao: %f segundos (%f ms)\n", tempo_gasto, tempo_gasto * 1000.0);

    FILE *f = fopen(saida, "w");
    if (!f) { perror(saida); free(a); return -1; }
    for (size_t i = 0; i < n; i++) fprintf(f, "%d\n", a[i]);
    fclose(f);

    printf("%zu numeros ordenados -> %s\n", n, saida);
    free(a);
    return 0;
}

int main(int argc, char **argv)
{
    /* Uso: ./counting_sort [arquivo_entrada] [arquivo_saida]
     * Sem argumentos, procura listaOdenada.txt na pasta atual. */
    const char *entrada = argc > 1 ? argv[1] : "listaOdenada.txt";
    const char *saida   = argc > 2 ? argv[2] : "listaOrdenada_saida.txt";
    return ordenar_arquivo(entrada, saida) == 0 ? 0 : 1;
}
 

