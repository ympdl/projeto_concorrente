#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define MAX_THREADS 16     // Número máximo de threads permitidas
#define MAX_LINE 256       // Tamanho máximo de uma linha do CSV

// Vetores globais para armazenar os valores de X e Y
double *X, *Y;

// Quantidade de pontos (linhas válidas do dataset)
long N = 0;

// Número de threads que o usuário vai escolher
int numThreads;

// Estrutura que guarda as somas parciais de cada thread
typedef struct {
    double somaX, somaY, somaX2, somaXY;
} Parcial;

// Vetor global onde cada thread guarda suas somas parciais
Parcial parciais[MAX_THREADS];


// =========================== FUNÇÃO THREAD ===========================
// Cada thread executa esta função para calcular as somas parciais
void *calcula_somas(void *arg) {
    long id = (long)arg;  // ID da thread (0, 1, 2, ..., numThreads-1)

    // Calcula o intervalo (faixa) de índices que essa thread vai processar
    long inicio = id * (N / numThreads);
    long fim = (id == numThreads - 1) ? N : inicio + (N / numThreads);

    // Variáveis locais para acumular as somas
    double somaX = 0, somaY = 0, somaX2 = 0, somaXY = 0;

    // Cada thread percorre apenas sua parte do vetor (divisão por blocos)
    for (long i = inicio; i < fim; i++) {
        somaX  += X[i];         // soma dos valores de X
        somaY  += Y[i];         // soma dos valores de Y
        somaX2 += X[i] * X[i];  // soma dos quadrados de X
        somaXY += X[i] * Y[i];  // soma do produto X*Y
    }

    // Guarda os resultados parciais no vetor global "parciais"
    parciais[id].somaX = somaX;
    parciais[id].somaY = somaY;
    parciais[id].somaX2 = somaX2;
    parciais[id].somaXY = somaXY;

    // Finaliza a thread
    pthread_exit(NULL);
}


// =========================== FUNÇÃO PRINCIPAL ===========================
int main(int argc, char *argv[]) {
    // Verifica se o usuário passou os parâmetros corretos
    if (argc < 3) {
        printf("Uso: %s <arquivo.csv> <num_threads>\n", argv[0]);
        return 1;
    }

    char *nomeArquivo = argv[1];
    numThreads = atoi(argv[2]);

    // Limita o número de threads ao máximo permitido
    if (numThreads > MAX_THREADS) numThreads = MAX_THREADS;

    // ==================== LEITURA DO ARQUIVO CSV ====================

    FILE *arquivo = fopen(nomeArquivo, "r");
    if (!arquivo) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    char linha[MAX_LINE];

    // Ignora o cabeçalho (por exemplo, "x,y")
    if (fgets(linha, MAX_LINE, arquivo) == NULL) {
        fprintf(stderr, "Erro: arquivo vazio ou sem cabeçalho.\n");
        fclose(arquivo);
        return 1;
    }

    // Conta o número de linhas (pontos de dados)
    while (fgets(linha, MAX_LINE, arquivo))
        N++;

    if (ferror(arquivo)) {
        fprintf(stderr, "Aviso: erro ao ler o arquivo '%s'\n", nomeArquivo);
    }
    fclose(arquivo);

    // Se não houver dados, encerra
    if (N <= 0) {
        fprintf(stderr, "Erro: nenhum dado encontrado no arquivo '%s'\n", nomeArquivo);
        return 1;
    }

    // ==================== ALOCAÇÃO DE MEMÓRIA ====================

    X = malloc(N * sizeof(double));
    Y = malloc(N * sizeof(double));

    if (!X || !Y) {
        fprintf(stderr, "Erro: falha ao alocar memória para %ld pontos\n", N);
        free(X); free(Y);
        return 1;
    }

    // ==================== LEITURA REAL DOS DADOS ====================

    arquivo = fopen(nomeArquivo, "r");
    if (!arquivo) {
        perror("Erro ao reabrir o arquivo");
        free(X); free(Y);
        return 1;
    }

    // Ignora novamente o cabeçalho
    fgets(linha, MAX_LINE, arquivo);

    long i = 0;
    while (fgets(linha, MAX_LINE, arquivo)) {
        double x, y;

        // Lê os valores separados por vírgula (ex: "2.3,4.5")
        if (sscanf(linha, "%lf,%lf", &x, &y) == 2) {
            if (i < N) {
                X[i] = x;
                Y[i] = y;
                i++;
            }
        }
    }

    fclose(arquivo);

    // Ajusta N se alguma linha for inválida
    if (i != N) {
        fprintf(stderr, "Aviso: contei %ld linhas mas li %ld pontos. Ajustando N = %ld\n", N, i, i);
        N = i;
    }

    if (numThreads <= 0) numThreads = 1;

    // ==================== CRIAÇÃO DAS THREADS ====================

    pthread_t threads[numThreads];

    // Cada thread vai processar uma parte diferente do vetor
    for (long t = 0; t < numThreads; t++)
        pthread_create(&threads[t], NULL, calcula_somas, (void *)t);

    // Aguarda todas as threads terminarem
    for (int t = 0; t < numThreads; t++)
        pthread_join(threads[t], NULL);

    // ==================== ETAPA DE REDUÇÃO ====================

    // Agora somamos todas as parciais em uma soma total
    double somaX = 0, somaY = 0, somaX2 = 0, somaXY = 0;

    for (int t = 0; t < numThreads; t++) {
        somaX  += parciais[t].somaX;
        somaY  += parciais[t].somaY;
        somaX2 += parciais[t].somaX2;
        somaXY += parciais[t].somaXY;
    }

    // ==================== CÁLCULO DA REGRESSÃO ====================

    double B = (N * somaXY - somaX * somaY) / (N * somaX2 - somaX * somaX);
    double A = (somaY - B * somaX) / N;

    // ==================== EXIBE RESULTADOS ====================

    printf("\n=== RESULTADOS ===\n");
    printf("Número de pontos: %ld\n", N);
    printf("A (intercepto): %.6f\n", A);
    printf("B (inclinação): %.6f\n", B);

    // Libera memória
    free(X);
    free(Y);

    return 0;
}
