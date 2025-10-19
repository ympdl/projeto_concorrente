#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "timer.h"

double *X, *Y;
long N = 0;
int numThreads;

typedef struct {
    double somaX, somaY, somaX2, somaXY;
} Parcial;

Parcial *parciais;

void *calcula_somas(void *arg) {
    long id = (long)arg;  
    long inicio = id * (N / numThreads);
    long fim = (id == numThreads - 1) ? N : inicio + (N / numThreads);

    // Variáveis em registradores para otimização
    register double somaX = 0, somaY = 0, somaX2 = 0, somaXY = 0;
    register double x_val, y_val;
    
    for (long i = inicio; i < fim; i++) {
        x_val = X[i];
        y_val = Y[i];
        somaX  += x_val;
        somaY  += y_val;
        somaX2 += x_val * x_val;
        somaXY += x_val * y_val;
    }

    parciais[id].somaX = somaX;
    parciais[id].somaY = somaY;
    parciais[id].somaX2 = somaX2;
    parciais[id].somaXY = somaXY;

    pthread_exit(NULL);
}

// ==================== FUNÇÃO DE PREVISÃO ====================
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

int main(int argc, char *argv[]) {
    double inicio, fim;

    if (argc < 3) {
        printf("Uso: %s <arquivo.csv> <num_threads>\n", argv[0]);
        return 1;
    }

    GET_TIME(inicio);  // MEDIÇÃO DO TEMPO INICIAL

    char *nomeArquivo = argv[1];
    numThreads = atoi(argv[2]);

    FILE *arquivo = fopen(nomeArquivo, "r");
    if (!arquivo) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    char linha[256];
    
    // PULA CABEÇALHO
    if (fgets(linha, sizeof(linha), arquivo) == NULL) {
        fprintf(stderr, "Erro: arquivo vazio\n");
        fclose(arquivo);
        return 1;
    }

    int capacity = 10000;  // Capacidade inicial
    X = malloc(capacity * sizeof(double));
    Y = malloc(capacity * sizeof(double));
    
    if (!X || !Y) {
        fprintf(stderr, "Erro ao alocar memória inicial\n");
        fclose(arquivo);
        return 1;
    }

    // LÊ DADOS DO ARQUIVO
    while (fgets(linha, sizeof(linha), arquivo)) {
        double x, y;
        if (sscanf(linha, "%lf,%lf", &x, &y) == 2) {
            // Realoca se necessário
            if (N >= capacity) {
                capacity *= 2;
                X = realloc(X, capacity * sizeof(double));
                Y = realloc(Y, capacity * sizeof(double));
                if (!X || !Y) {
                    fprintf(stderr, "Erro ao realocar memória\n");
                    fclose(arquivo);
                    return 1;
                }
            }
            X[N] = x;
            Y[N] = y;
            N++;
        }
    }
    fclose(arquivo);

    // Aloca parciais
    parciais = malloc(numThreads * sizeof(Parcial));
    if (!parciais) {
        fprintf(stderr, "Erro ao alocar parciais\n");
        free(X); free(Y);
        return 1;
    }

    // Ajusta número de threads se necessário
    if (numThreads > N) {
        numThreads = N;
    }

    // Cria e executa threads
    pthread_t threads[numThreads];
    for (long t = 0; t < numThreads; t++) {
        pthread_create(&threads[t], NULL, calcula_somas, (void *)t);
    }

    for (int t = 0; t < numThreads; t++) {
        pthread_join(threads[t], NULL);
    }

    // Reduz resultados
    double somaX = 0, somaY = 0, somaX2 = 0, somaXY = 0;
    for (int t = 0; t < numThreads; t++) {
        somaX  += parciais[t].somaX;
        somaY  += parciais[t].somaY;
        somaX2 += parciais[t].somaX2;
        somaXY += parciais[t].somaXY;
    }

    // Cálculo final
    double B = (N * somaXY - somaX * somaY) / (N * somaX2 - somaX * somaX);
    double A = (somaY - B * somaX) / N;

    GET_TIME(fim);

    printf("\n=== RESULTADOS ===\n");
    printf("Numero de pontos: %ld\n", N);
    printf("Threads usadas: %d\n", numThreads);
    printf("A (intercepto): %.6f\n", A);
    printf("B (inclinacao): %.6f\n", B);

    printf("\n=== TEMPOS DE EXECUCAO ===\n");
    printf("Tempo total: %f segundos\n", fim - inicio);

    // Modo de previsão interativo
    prever_valores(A, B);

    free(X);
    free(Y);
    free(parciais);
    return 0;
}