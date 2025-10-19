#include <stdio.h>
#include <stdlib.h>
#include "timer.h"   // Arquivo de medição de tempo de 

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <arquivo.csv>\n", argv[0]);
        return 1;
    }

    FILE *arquivo;
    char linha[100];
    double x, y;
    double somaX = 0, somaY = 0, somaXY = 0, somaX2 = 0;
    long long n = 0;
    double a, b;
    double start, finish, delta;

    // Início da medição de tempo
    GET_TIME(start);

    // Abre o arquivo CSV passado por argumento
    arquivo = fopen(argv[1], "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s!\n", argv[1]);
        return 1;
    }

    // Lê linha por linha (espera formato x,y)
    while (fgets(linha, sizeof(linha), arquivo)) {
        if (sscanf(linha, "%lf,%lf", &x, &y) == 2) {
            somaX += x;
            somaY += y;
            somaXY += x * y;
            somaX2 += x * x;
            n++;
        }
    }

    fclose(arquivo);

    // Calcula os coeficientes da regressão linear
    b = (n * somaXY - somaX * somaY) / (n * somaX2 - somaX * somaX);
    a = (somaY - b * somaX) / n;

    // Fim da medição de tempo
    GET_TIME(finish);
    delta = finish - start;

    // Resultados
    printf("=== Regressão Linear Sequencial ===\n");
    printf("Arquivo: %s\n", argv[1]);
    printf("Número de pontos: %lld\n", n);
    printf("Equação da reta: y = %.6lf + %.6lfx\n", a, b);
    printf("Tempo de execução: %.6f segundos\n", delta);

    return 0;
}
