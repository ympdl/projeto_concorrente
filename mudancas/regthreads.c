#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "timer.h"

#define MAX_LINHA 1024

typedef struct {
    double *X;
    double *Y;
    int inicio;
    int fim;
    double sumX;
    double sumY;
    double sumXY;
    double sumXX;
    double mse;
    double A;
    double B;
} ThreadData;

// Função para ler dados do CSV
int carregar_dados(const char *arquivo, double **X, double **Y, int n_amostras) {
    FILE *fp = fopen(arquivo, "r");
    if (!fp) {
        printf("Erro ao abrir o arquivo %s\n", arquivo);
        return -1;
    }

    char linha[MAX_LINHA];
    int count = 0;
    int capacidade = n_amostras > 0 ? n_amostras : 1000;
    *X = (double *)malloc(capacidade * sizeof(double));
    *Y = (double *)malloc(capacidade * sizeof(double));

    while (fgets(linha, MAX_LINHA, fp)) {
        char *token = strtok(linha, ",");
        if (!token) continue;
        double x = atof(token);

        token = strtok(NULL, ",");
        if (!token) continue;
        double y = atof(token);

        if (count >= capacidade) {
            capacidade *= 2;
            *X = (double *)realloc(*X, capacidade * sizeof(double));
            *Y = (double *)realloc(*Y, capacidade * sizeof(double));
        }

        (*X)[count] = x;
        (*Y)[count] = y;
        count++;

        if (n_amostras > 0 && count >= n_amostras) break;
    }

    fclose(fp);
    return count;
}

// Thread calcula soma parcial para regressão linear
void *soma_parcial(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
    for (int i = data->inicio; i < data->fim; i++) {
        sumX += data->X[i];
        sumY += data->Y[i];
        sumXY += data->X[i] * data->Y[i];
        sumXX += data->X[i] * data->X[i];
    }
    data->sumX = sumX;
    data->sumY = sumY;
    data->sumXY = sumXY;
    data->sumXX = sumXX;
    return NULL;
}

// Thread calcula parte do MSE
void *mse_parcial(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    double mse = 0;
    for (int i = data->inicio; i < data->fim; i++) {
        double pred = data->A + data->B * data->X[i];
        mse += (data->Y[i] - pred) * (data->Y[i] - pred);
    }
    data->mse = mse;
    return NULL;
}

// Regressão linear com threads e cálculo de MSE
void regressao_linear_thread(double *X, double *Y, int n, int n_threads, double *A_out, double *B_out, double *mse_out) {
    pthread_t threads[n_threads];
    ThreadData thread_data[n_threads];

    // 1. Soma parcial
    int base = n / n_threads;
    int resto = n % n_threads;
    for (int i = 0, start = 0; i < n_threads; i++) {
        int end = start + base + (i < resto ? 1 : 0);
        thread_data[i].X = X;
        thread_data[i].Y = Y;
        thread_data[i].inicio = start;
        thread_data[i].fim = end;
        pthread_create(&threads[i], NULL, soma_parcial, &thread_data[i]);
        start = end;
    }

    double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
        sumX += thread_data[i].sumX;
        sumY += thread_data[i].sumY;
        sumXY += thread_data[i].sumXY;
        sumXX += thread_data[i].sumXX;
    }

    double B = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
    double A = (sumY - B * sumX) / n;

    // 2. Calcula MSE com threads
    for (int i = 0, start = 0; i < n_threads; i++) {
        int end = start + base + (i < resto ? 1 : 0);
        thread_data[i].A = A;
        thread_data[i].B = B;
        thread_data[i].inicio = start;
        thread_data[i].fim = end;
        pthread_create(&threads[i], NULL, mse_parcial, &thread_data[i]);
        start = end;
    }

    double mse = 0;
    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
        mse += thread_data[i].mse;
    }
    mse /= n;

    *A_out = A;
    *B_out = B;
    *mse_out = mse;
}

// Função para testar modelos com threads
void testar_modelo(const char *arquivo, int tamanhos[], int n_tamanhos, int threads_arr[], int n_threads_array, int n_iter) {
    // Para resumo final: armazenar médias de cada par tamanho/threads
    double **resumo_A = malloc(n_tamanhos * sizeof(double *));
    double **resumo_B = malloc(n_tamanhos * sizeof(double *));
    double **resumo_mse = malloc(n_tamanhos * sizeof(double *));
    double **resumo_tempo = malloc(n_tamanhos * sizeof(double *));
    for(int t = 0; t < n_tamanhos; t++){
        resumo_A[t] = malloc(n_threads_array * sizeof(double));
        resumo_B[t] = malloc(n_threads_array * sizeof(double));
        resumo_mse[t] = malloc(n_threads_array * sizeof(double));
        resumo_tempo[t] = malloc(n_threads_array * sizeof(double));
    }

    for (int t = 0; t < n_tamanhos; t++) {
        int n = tamanhos[t];
        for (int th = 0; th < n_threads_array; th++) {
            int n_threads = threads_arr[th];
            printf("\n--- Tamanho: %d, Threads: %d ---\n", n, n_threads);

            double A_sum = 0, B_sum = 0, mse_sum = 0, tempo_sum = 0;

            for (int i = 0; i < n_iter; i++) {
                double *X = NULL, *Y = NULL;
                int n_dados = carregar_dados(arquivo, &X, &Y, n);
                if (n_dados <= 0) { free(X); free(Y); continue; }

                double A, B, mse;
                double inicio, fim;
                GET_TIME(inicio);
                regressao_linear_thread(X, Y, n_dados, n_threads, &A, &B, &mse);
                GET_TIME(fim);
                double tempo = fim - inicio;

                printf("Iter %d: A=%.6f, B=%.6f, MSE=%.8f, Tempo=%.6f s\n", i+1, A, B, mse, tempo);

                A_sum += A;
                B_sum += B;
                mse_sum += mse;
                tempo_sum += tempo;

                free(X);
                free(Y);
            }

            // Médias por tamanho/threads
            double A_media = A_sum / n_iter;
            double B_media = B_sum / n_iter;
            double mse_media = mse_sum / n_iter;
            double tempo_media = tempo_sum / n_iter;

            resumo_A[t][th] = A_media;
            resumo_B[t][th] = B_media;
            resumo_mse[t][th] = mse_media;
            resumo_tempo[t][th] = tempo_media;

            printf("=== Médias para tamanho=%d, threads=%d ===\n", n, n_threads);
            printf("A=%.6f, B=%.6f, MSE=%.8f, Tempo=%.6f s\n", A_media, B_media, mse_media, tempo_media);
        }
    }

    // Resumo final
// Impressão do resumo final em blocos por número de threads
    printf("\n=== RESUMO FINAL POR NÚMERO DE THREADS ===\n");

    for (int th = 0; th < n_threads_array; th++) {
        int n_threads = threads_arr[th];
        printf("\n--- Threads = %d ---\n", n_threads);
        printf("N\tA\t\tB\t\tMSE\t\tTempo (s)\n");

        for (int t = 0; t < n_tamanhos; t++) {
            printf("%d\t%.6f\t%.6f\t%.8f\t%.4f\n",
                tamanhos[t],
                resumo_A[t][th],
                resumo_B[t][th],
                resumo_mse[t][th],
                resumo_tempo[t][th]);
        }
    }

    // Libera memória do resumo
    for(int t = 0; t < n_tamanhos; t++){
        free(resumo_A[t]);
        free(resumo_B[t]);
        free(resumo_mse[t]);
        free(resumo_tempo[t]);
    }
    free(resumo_A);
    free(resumo_B);
    free(resumo_mse);
    free(resumo_tempo);
}

int main() {
    const char *arquivo = "dados.csv";
    int tamanhos[] = {10000, 100000, 1000000, 10000000};
    int n_tamanhos = sizeof(tamanhos)/sizeof(tamanhos[0]);

    int threads_arr[] = {2, 4, 8};
    int n_threads_array = sizeof(threads_arr)/sizeof(threads_arr[0]);

    int n_iter = 10;

    testar_modelo(arquivo, tamanhos, n_tamanhos, threads_arr, n_threads_array, n_iter);

    return 0;
}
