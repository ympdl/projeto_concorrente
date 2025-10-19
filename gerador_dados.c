#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <arquivo_saida.csv> <num_amostras> [ruido]\n", argv[0]);
        printf("Exemplo: %s dados.csv 100000 0.5\n", argv[0]);
        return 1;
    }

    char *nomeArquivo = argv[1];
    long N = atol(argv[2]);
    double ruido = (argc >= 4) ? atof(argv[3]) : 0.0;

    FILE *arquivo = fopen(nomeArquivo, "w");
    if (!arquivo) {
        perror("Erro ao criar o arquivo");
        return 1;
    }

    srand(time(NULL));

    // Cabeçalho
    fprintf(arquivo, "x,y\n");

    // Parâmetros reais da regressão (ex: y = a + b*x)
    double a = 2.0;
    double b = 3.5;

    for (long i = 0; i < N; i++) {
        double x = (double)i / 10.0; // valores de X crescentes
        double ruidoAleatorio = ruido * ((rand() % 1000) / 1000.0 - 0.5) * 2; // [-ruido, +ruido]
        double y = a + b * x + ruidoAleatorio;
        fprintf(arquivo, "%.6f,%.6f\n", x, y);
    }

    fclose(arquivo);
    printf("Arquivo '%s' gerado com %ld amostras (ruido = %.2f)\n", nomeArquivo, N, ruido);
    return 0;
}
