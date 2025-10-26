#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_LINHA 1024

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

// Regressão linear manual
void regressao_linear(double *X, double *Y, int n, double *A, double *B) {
    double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
    for (int i = 0; i < n; i++) {
        sumX += X[i];
        sumY += Y[i];
        sumXY += X[i] * Y[i];
        sumXX += X[i] * X[i];
    }
    *B = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
    *A = (sumY - (*B) * sumX) / n;
}

// Calcula MSE
double calcular_mse(double *X, double *Y, int n, double A, double B) {
    double mse = 0;
    for (int i = 0; i < n; i++) {
        double pred = A + B * X[i];
        mse += (Y[i] - pred) * (Y[i] - pred);
    }
    return mse / n;
}

// Função para testar o modelo
void testar_modelo(const char *arquivo, int tamanhos[], int n_tamanhos, int n_iter) {
    // Arrays para armazenar médias finais
    double *A_media = (double *)malloc(n_tamanhos * sizeof(double));
    double *B_media = (double *)malloc(n_tamanhos * sizeof(double));
    double *mse_media = (double *)malloc(n_tamanhos * sizeof(double));
    double *tempo_media = (double *)malloc(n_tamanhos * sizeof(double));

    for (int t = 0; t < n_tamanhos; t++) {
        int n = tamanhos[t];
        printf("\n--- Testando com %d pontos ---\n", n);

        double A_sum = 0, B_sum = 0, mse_sum = 0, tempo_sum = 0;

        for (int i = 0; i < n_iter; i++) {
            double *X = NULL, *Y = NULL;
            int n_dados = carregar_dados(arquivo, &X, &Y, n);
            if (n_dados <= 0) { free(X); free(Y); continue; }

            double A, B, mse;
            double inicio, fim;
            GET_TIME(inicio);

            regressao_linear(X, Y, n_dados, &A, &B);
            mse = calcular_mse(X, Y, n_dados, A, B);

            GET_TIME(fim);
            double tempo = fim - inicio;

            printf("Iteração %d: A=%.6f, B=%.6f, MSE=%.8f, Tempo=%.6f s\n", i+1, A, B, mse, tempo);

            A_sum += A;
            B_sum += B;
            mse_sum += mse;
            tempo_sum += tempo;

            free(X);
            free(Y);
        }

        // Calcula média das métricas
        A_media[t] = A_sum / n_iter;
        B_media[t] = B_sum / n_iter;
        mse_media[t] = mse_sum / n_iter;
        tempo_media[t] = tempo_sum / n_iter;

        printf("=== Médias para %d pontos ===\n", n);
        printf("A=%.6f, B=%.6f, MSE=%.8f, Tempo=%.6f s\n",
               A_media[t], B_media[t], mse_media[t], tempo_media[t]);
    }

    // Resumo final
    printf("\n=== RESUMO FINAL ===\n");
    printf("N\tA\t\tB\t\tMSE\t\tTempo (s)\n");
    for (int t = 0; t < n_tamanhos; t++) {
        printf("%d\t%.6f\t%.6f\t%.8f\t%.4f\n",
               tamanhos[t], A_media[t], B_media[t], mse_media[t], tempo_media[t]);
    }

    free(A_media);
    free(B_media);
    free(mse_media);
    free(tempo_media);
}

int main() {
    const char *arquivo = "dados.csv";
    int tamanhos[] = {10000, 100000, 1000000, 10000000};
    int n_tamanhos = sizeof(tamanhos)/sizeof(tamanhos[0]);
    int n_iter = 10;

    testar_modelo(arquivo, tamanhos, n_tamanhos, n_iter);

    return 0;
}
