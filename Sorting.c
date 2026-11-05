#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

/*O código precisa ser comentado, *assim**/
//vou usar Counting sort (com Radix sort de reserva quando o intervalo e muito grande)
//compilado com gcc 

//gcc -O2 -o counting_sort Sorting.c
//./counting_sort

/* Limite do vetor de contagem: se m = max - min + 1 passar de
 * FATOR_INTERVALO * n + FOLGA_INTERVALO, o counting sort gastaria memoria
 * demais em relacao ao tamanho da entrada e usamos o radix sort. */
#define FATOR_INTERVALO 4ULL
#define FOLGA_INTERVALO (1ULL << 20)

/* Le todos os inteiros (um por linha) do arquivo em 'caminho'.
 * Retorna um vetor alocado com malloc (o chamador faz free) e grava o tamanho em *n.
 * Valida cada valor: token que nao seja inteiro ou que nao caiba em int
 * gera mensagem com o numero da linha e retorna NULL.
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
    size_t linha = 1;

    /* pula o BOM UTF-8 (EF BB BF), comum em arquivos salvos no Windows */
    if ((unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB &&
        (unsigned char)p[2] == 0xBF) p += 3;

    while (*p) {
        /* pula espacos em branco (inclui \r e \n), contando as linhas */
        while (*p && isspace((unsigned char)*p)) {
            if (*p == '\n') linha++;
            p++;
        }
        if (!*p) break;

        /* converte com strtoll e confere se o token inteiro foi consumido */
        char *fim;
        errno = 0;
        long long v = strtoll(p, &fim, 10);

        /* "-" solto, "3.5", "12abc" etc: nao e um inteiro valido */
        if (fim == p || (*fim && !isspace((unsigned char)*fim))) {
            fprintf(stderr, "%s:%zu: valor invalido (esperado um inteiro)\n", caminho, linha);
            free(a); free(buf); return NULL;
        }
        /* estouro de long long ou fora da faixa de int */
        if (errno == ERANGE || v < INT_MIN || v > INT_MAX) {
            fprintf(stderr, "%s:%zu: valor fora do intervalo de int\n", caminho, linha);
            free(a); free(buf); return NULL;
        }

        if (cnt == cap) {
            cap *= 2;
            int *tmp = realloc(a, cap * sizeof *a);
            if (!tmp) { free(a); free(buf); return NULL; }
            a = tmp;
        }
        a[cnt++] = (int)v;
        p = fim;
    }
    free(buf);
    *n = cnt;
    return a;
}

/* Radix sort LSD, base 256 (4 passadas): O(n), memoria extra = n ints,
 * independente do intervalo dos valores. Usado quando o counting sort
 * precisaria de um vetor de contagem grande demais. Ordena 'a' no proprio vetor. */
static int radix_sort(int *a, size_t n)
{
    if (n < 2) return 0;

    int *aux = malloc(n * sizeof *aux);
    if (!aux) { fprintf(stderr, "sem memoria para o vetor auxiliar\n"); return -1; }

    /* histogramas dos 4 bytes de cada chave, calculados em uma unica leitura.
     * Inverter o bit de sinal faz os negativos ficarem antes dos positivos. */
    size_t h[4][256] = {{0}};
    for (size_t i = 0; i < n; i++) {
        uint32_t k = (uint32_t)a[i] ^ 0x80000000u;
        h[0][k & 0xFF]++;
        h[1][(k >> 8) & 0xFF]++;
        h[2][(k >> 16) & 0xFF]++;
        h[3][k >> 24]++;
    }

    int *src = a, *dst = aux;
    uint32_t k0 = (uint32_t)a[0] ^ 0x80000000u;

    for (int b = 0; b < 4; b++) {
        int sh = 8 * b;

        /* se todos os valores tem o mesmo byte nesta posicao, a passada nao muda nada */
        if (h[b][(k0 >> sh) & 0xFF] == n) continue;

        /* soma de prefixos: posicao inicial de cada digito no destino */
        size_t soma = 0;
        for (int d = 0; d < 256; d++) {
            size_t t = h[b][d];
            h[b][d] = soma;
            soma += t;
        }

        /* distribui de forma estavel (preserva a ordem das passadas anteriores) */
        for (size_t i = 0; i < n; i++) {
            uint32_t k = (uint32_t)src[i] ^ 0x80000000u;
            dst[h[b][(k >> sh) & 0xFF]++] = src[i];
        }

        int *t = src; src = dst; dst = t;
    }

    /* o resultado pode ter ficado no vetor auxiliar */
    if (src != a) memcpy(a, src, n * sizeof *a);

    free(aux);
    return 0;
}

/* Counting sort: O(n + m), onde m = max - min + 1. Ordena 'a' no proprio vetor.
 * Se m for desproporcional a n (vetor de contagem gigante), delega para o radix sort. */
int counting_sort(int *a, size_t n)
{
    if (n == 0) return 0;

    int min = a[0], max = a[0];
    for (size_t i = 1; i < n; i++) {
        if (a[i] < min) min = a[i];
        if (a[i] > max) max = a[i];
    }

    size_t m = (size_t)((long long)max - min + 1);

    /* intervalo grande demais para o tamanho da entrada: usa o radix sort.
     * Tambem cai aqui se n nao couber no contador de 32 bits. */
    if ((unsigned long long)m > FATOR_INTERVALO * n + FOLGA_INTERVALO ||
        (unsigned long long)n > UINT32_MAX)
        return radix_sort(a, n);

    /* contadores de 4 bytes: metade da memoria de size_t */
    uint32_t *c = calloc(m, sizeof *c);
    if (!c) { fprintf(stderr, "sem memoria para o vetor de contagem\n"); return -1; }

    for (size_t i = 0; i < n; i++) c[(size_t)((long long)a[i] - min)]++;

    size_t k = 0;
    for (size_t v = 0; v < m; v++)
        for (uint32_t j = c[v]; j > 0; j--)
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
     * Sem argumentos, procura listaOrdenada.txt na pasta atual. */
    const char *entrada = argc > 1 ? argv[1] : "listaOrdenada.txt";
    const char *saida   = argc > 2 ? argv[2] : "listaOrdenada_saida.txt";
    return ordenar_arquivo(entrada, saida) == 0 ? 0 : 1;
}
