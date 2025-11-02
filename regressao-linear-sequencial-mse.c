#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "timer.h" 

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
    if (argc < 2) {
        printf("Uso: %s <arquivo.csv>\n", argv[0]);
        return 1;
    }

    FILE *arquivo;
    char linha[256];
    double *X = NULL, *Y = NULL;
    long long N = 0;
    long long capacidade = 1000; // capacidade inicial
    double somaX = 0, somaY = 0, somaXY = 0, somaX2 = 0;
    double a, b, mse;
    double start, finish, delta, inicio_total, fim_total;

    GET_TIME(inicio_total);  // MEDIÇÃO DO TEMPO TOTAL INICIAL

    // Aloca espaço inicial
    X = malloc(capacidade * sizeof(double));
    Y = malloc(capacidade * sizeof(double));
    if (!X || !Y) {
        fprintf(stderr, "Erro ao alocar memória\n");
        return 1;
    }

    // Abre o arquivo CSV passado por argumento
    arquivo = fopen(argv[1], "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s!\n", argv[1]);
        return 1;
    }

      // PULA O CABEÇALHO
    if (fgets(linha, sizeof(linha), arquivo) == NULL) {
        fprintf(stderr, "Erro: arquivo vazio ou sem cabeçalho\n");
        fclose(arquivo);
        free(X); free(Y);
        return 1;
    }

    // ==================== LEITURA IGUAL À DO PARALELO ====================
    while (fgets(linha, sizeof(linha), arquivo)) {
        double x, y;
        if (sscanf(linha, "%lf,%lf", &x, &y) == 2) {
            // Realoca se necessário
            if (N >= capacidade) {
                capacidade *= 2;
                X = realloc(X, capacidade * sizeof(double));
                Y = realloc(Y, capacidade * sizeof(double));
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
    // ==================== FIM DA LEITURA ====================
    
    GET_TIME(start);  // MEDIÇÃO DO TEMPO INICIAL DA REGRESSÃO
    
    // Calcula os coeficientes da regressão linear
    for (long long i = 0; i < N; i++) {
        somaX += X[i];
        somaY += Y[i];
        somaXY += X[i] * Y[i];
        somaX2 += X[i] * X[i];
    }

    b = (N * somaXY - somaX * somaY) / (N * somaX2 - somaX * somaX);
    a = (somaY - b * somaX) / N;

    GET_TIME(finish);  // FIM DA MEDIÇÃO DA REGRESSÃO
    
    // ==================== CÁLCULO DO MSE ====================
    double soma_erro_quad = 0.0;
    // Calcula o Erro Quadrático Médio (MSE)
    for (long long i = 0; i < N; i++) {
        double y_prev = a + b * X[i];      // Valor previsto pela regressão
        double erro = Y[i] - y_prev;       // Diferença entre valor real e previsto
        soma_erro_quad += erro * erro;     // Acumula o quadrado do erro
    }
    
    mse = soma_erro_quad / N;              // MSE = média dos erros quadráticos

    GET_TIME(finish);  // FIM DA MEDIÇÃO DA REGRESSÃO
    GET_TIME(fim_total);  // MEDIÇÃO DO TEMPO TOTAL FINAL
    
    delta = finish - start;  // Tempo apenas da regressão

    // Resultados
    printf("=== Regressão Linear Sequencial ===\n");
    printf("Arquivo: %s\n", argv[1]);
    printf("Número de pontos: %lld\n", N);
    printf("Equação da reta: y = %.6lf + %.6lfx\n", a, b);
    printf("MSE (Erro Quadrático Médio): %.10f\n", mse);
    printf("\n=== Tempos de Execução ===\n");
    printf("Tempo da regressão: %.6f segundos\n", delta);
    printf("Tempo total do programa: %.6f segundos\n", fim_total - inicio_total);

    // Libera memória
    free(X);
    free(Y);

    // Modo de previsão interativo
    prever_valores(a, b);

    return 0;
}