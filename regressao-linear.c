#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define MAX_THREADS 16
#define MAX_LINE 256

double *X, *Y;
long N = 0;
int numThreads;

typedef struct {
    double somaX, somaY, somaX2, somaXY;
} Parcial;

Parcial parciais[MAX_THREADS];

// ==================== FUNÇÃO DE CÁLCULO PARCIAL ====================
void *calcula_somas(void *arg) {
    long id = (long)arg;  
    
    long inicio = id * (N / numThreads);
    long fim = (id == numThreads - 1) ? N : inicio + (N / numThreads);

    double somaX = 0, somaY = 0, somaX2 = 0, somaXY = 0;

    for (long i = inicio; i < fim; i++) {
        somaX  += X[i];
        somaY  += Y[i];
        somaX2 += X[i] * X[i];
        somaXY += X[i] * Y[i];
    }

    parciais[id].somaX = somaX;
    parciais[id].somaY = somaY;
    parciais[id].somaX2 = somaX2;
    parciais[id].somaXY = somaXY;

    pthread_exit(NULL);
}

// ==================== NOVA FUNÇÃO: PREVISÃO DE VALORES ====================
void prever_valores(double A, double B) {
    char entrada[64];
    double x;

    printf("\n=== MODO DE PREVISAO ===\n");
    printf("Digite um valor de X para prever Y (ou 'q' para sair)\n");

    while (1) {
        printf("X = ");
        if (scanf("%s", entrada) != 1)
            break;

        if (strcmp(entrada, "q") == 0 || strcmp(entrada, "sair") == 0)
            break;

        if (sscanf(entrada, "%lf", &x) == 1) {
            double y_prev = A + B * x;
            printf("-> Y previsto = %.6f\n", y_prev);
        } else {
            printf("Entrada invalida. Digite um numero ou 'q' para sair.\n");
        }
    }

    printf("Saindo do modo de previsao.\n");
}

// =========================== FUNÇÃO PRINCIPAL ===========================
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <arquivo.csv> <num_threads>\n", argv[0]);
        return 1;
    }

    char *nomeArquivo = argv[1];
    numThreads = atoi(argv[2]);
    if (numThreads > MAX_THREADS) numThreads = MAX_THREADS;
    if (numThreads <= 0) numThreads = 1;

    FILE *arquivo = fopen(nomeArquivo, "r");
    if (!arquivo) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    char linha[MAX_LINE];

    // Ignora o cabeçalho
    if (fgets(linha, MAX_LINE, arquivo) == NULL) {
        fprintf(stderr, "Erro: arquivo vazio ou sem cabeçalho.\n");
        fclose(arquivo);
        return 1;
    }

    while (fgets(linha, MAX_LINE, arquivo))
        N++;
    fclose(arquivo);

    if (N <= 0) {
        fprintf(stderr, "Erro: nenhum dado encontrado.\n");
        return 1;
    }

    X = malloc(N * sizeof(double));
    Y = malloc(N * sizeof(double));
    if (!X || !Y) {
        fprintf(stderr, "Erro ao alocar memória.\n");
        free(X); free(Y);
        return 1;
    }

    arquivo = fopen(nomeArquivo, "r");
    fgets(linha, MAX_LINE, arquivo); // ignora cabeçalho

    long i = 0;
    while (fgets(linha, MAX_LINE, arquivo)) {
        double x, y;
        if (sscanf(linha, "%lf,%lf", &x, &y) == 2) {
            X[i] = x;
            Y[i] = y;
            i++;
        }
    }
    fclose(arquivo);
    N = i;

    pthread_t threads[numThreads];
    for (long t = 0; t < numThreads; t++)
        pthread_create(&threads[t], NULL, calcula_somas, (void *)t);
    for (int t = 0; t < numThreads; t++)
        pthread_join(threads[t], NULL);

    double somaX = 0, somaY = 0, somaX2 = 0, somaXY = 0;
    for (int t = 0; t < numThreads; t++) {
        somaX  += parciais[t].somaX;
        somaY  += parciais[t].somaY;
        somaX2 += parciais[t].somaX2;
        somaXY += parciais[t].somaXY;
    }

    double B = (N * somaXY - somaX * somaY) / (N * somaX2 - somaX * somaX);
    double A = (somaY - B * somaX) / N;

    printf("\n=== RESULTADOS ===\n");
    printf("Numero de pontos: %ld\n", N);
    printf("A (intercepto): %.6f\n", A);
    printf("B (inclinacao): %.6f\n", B);

    // ==================== NOVA PARTE: MODO DE PREVISÃO ====================
    prever_valores(A, B);

    free(X);
    free(Y);
    return 0;
}