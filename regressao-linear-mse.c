#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
//#include <math.h>
#include "timer.h"

// Variáveis globais para armazenar os dados
// X e Y são arrays dinâmicos que armazenam os pontos (x,y) do arquivo CSV
double *X, *Y;
long N = 0;          // Número total de pontos lidos do arquivo
int numThreads;      // Número de threads definido pelo usuário

// Estrutura para armazenar resultados parciais de cada thread
// Cada thread calcula suas próprias somas localmente
typedef struct {
    double somaX, somaY, somaX2, somaXY;  // Somas parciais: Σx, Σy, Σx², Σxy
    double somaErroQuad;                   // Soma parcial do erro quadrático

} Parcial;

Parcial *parciais;  // Array de estruturas para armazenar resultados de cada thread

// Estrutura para passar parâmetros para as threads do MSE
typedef struct {
    long id;
    double A;  // Coeficiente linear
    double B;  // Coeficiente angular
    long inicio;
    long fim;
} ArgsMSE;

// ==================== FUNÇÃO EXECUTADA POR CADA THREAD ====================
void *calcula_somas(void *arg) {
    long id = (long)arg;  // ID da thread 
    
    long base = N / numThreads;
    long resto = N % numThreads;

    // Calcula o intervalo de trabalho para esta thread
    // Divisão por blocos: cada thread processa um segmento contíguo do array
    long inicio = id * base + (id < resto ? id : resto);
    long fim = inicio + base + (id < resto ? 1 : 0);

    // Variáveis locais para acumular as somas (evitam conflitos de memória)
    double somaX = 0, somaY = 0, somaX2 = 0, somaXY = 0;
    double somaErroQuad = 0;  // VARIÁVEL LOCAL PARA ACUMULAR O ERRO QUADRÁTICO
    double x_val, y_val;  // Variáveis temporárias para melhor performance
    
    // Percorre o segmento atribuído a esta thread
    for (long i = inicio; i < fim; i++) {
        x_val = X[i];  // Lê valor de X uma vez 
        y_val = Y[i];  // Lê valor de Y uma vez 
        somaX  += x_val;      // Acumula soma dos valores de X
        somaY  += y_val;      // Acumula soma dos valores de Y
        somaX2 += x_val * x_val;  // Acumula soma dos quadrados de X
        somaXY += x_val * y_val;  // Acumula soma dos produtos X*Y
    }

    // Armazena resultados parciais na estrutura compartilhada
    // Cada thread escreve em sua própria posição (evita corrida de dados)
    parciais[id].somaX = somaX;
    parciais[id].somaY = somaY;
    parciais[id].somaX2 = somaX2;
    parciais[id].somaXY = somaXY;
    parciais[id].somaErroQuad = somaErroQuad;  // Armazena soma parcial do erro quadrático

    pthread_exit(NULL); 
}

// ==================== CALCULO DO MSE EM PARALELO ====================
// Função executada por cada thread para calcular o Erro Quadrático Médio (MSE)
void *calcula_mse(void *arg) {
    // Converte o ponteiro genérico para a estrutura ArgsMSE que contém os parâmetros
    ArgsMSE *args = (ArgsMSE *)arg;
    
    // Variável local para acumular a soma dos erros quadráticos desta thread
    double somaErroQuad = 0;
    double x_val, y_val, y_prev, erro;
    
    // Loop que processa cada ponto do segmento atribuído a esta thread
    for (long i = args->inicio; i < args->fim; i++) {
        // Lê os valores reais de X e Y dos arrays globais
        x_val = X[i];
        y_val = Y[i];
        
        // Calcula o valor previsto Y usando a equação da regressão linear: ŷ = A + B*x
        // Usa os coeficientes A e B passados via estrutura de argumentos
        y_prev = args->A + args->B * x_val;
        
        // Calcula o erro (resíduo) = diferença entre valor real e valor previsto
        erro = y_val - y_prev;
        
        // Acumula o quadrado do erro - elimina sinais negativos e penaliza erros grandes
        somaErroQuad += erro * erro;
    }

    // Armazena o resultado parcial desta thread na estrutura compartilhada
    // Usa o ID da thread para escrever na posição correta do array parciais
    parciais[args->id].somaErroQuad = somaErroQuad;

    // Encerra a thread normalmente
    pthread_exit(NULL);
}

// ==================== FUNÇÃO DE PREVISÃO INTERATIVA ====================
void prever_valores(double A, double B) {
    char entrada[64];  // Buffer para entrada do usuário
    double x;          // Valor de X para previsão

    printf("\n=== MODO DE PREVISAO ===\n");
    printf("Digite um valor de X para prever Y (ou 'q' para sair)\n");

    // Loop infinito até o usuário digitar 'q' ou 'sair'
    while (1) {
        printf("X = ");
        if (scanf("%s", entrada) != 1)  // Lê entrada como string
            break;

        // Verifica se usuário quer sair
        if (strcmp(entrada, "q") == 0 || strcmp(entrada, "sair") == 0)
            break;

        // Tenta converter a entrada para número
        if (sscanf(entrada, "%lf", &x) == 1) {
            // Calcula Y previsto usando a equação da regressão linear: y = A + B*x
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
    // Variáveis para medição de tempo
    double inicio, fim, inicio_mse, fim_mse;           // Tempo dos cálculos paralelos
    double inicio_total, fim_total; // Tempo total do programa
    char linha[256];  // Buffer para ler cada linha do arquivo
    int capacidade = 10000;  // Capacidade inicial dos arrays

    // Verifica argumentos da linha de comando
    if (argc < 3) {
        printf("Uso: %s <arquivo.csv> <num_threads>\n", argv[0]);
        return 1;
    }

    char *nomeArquivo = argv[1];  // Nome do arquivo CSV
    numThreads = atoi(argv[2]);   // Converte número de threads para inteiro

    GET_TIME(inicio_total);  // Inicia medição do tempo TOTAL do programa
    
    // Abre arquivo para leitura
    FILE *arquivo = fopen(nomeArquivo, "r");
    if (!arquivo) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }
    
    // PULA CABEÇALHO - lê e descarta a primeira linha (ex: "x,y")
    if (fgets(linha, sizeof(linha), arquivo) == NULL) {
        fprintf(stderr, "Erro: arquivo vazio\n");
        fclose(arquivo);
        return 1;
    }

    // ==================== ALOCAÇÃO DINÂMICA INICIAL ====================
    X = malloc(capacidade * sizeof(double));
    Y = malloc(capacidade * sizeof(double));
    
    if (!X || !Y) {
        fprintf(stderr, "Erro ao alocar memória inicial\n");
        fclose(arquivo);
        return 1;
    }

    // ==================== LEITURA DO ARQUIVO CSV ====================
    // Lê cada linha do arquivo após o cabeçalho
    while (fgets(linha, sizeof(linha), arquivo)) {
        double x, y;
        // Tenta extrair dois números double separados por vírgula
        if (sscanf(linha, "%lf,%lf", &x, &y) == 2) {
            // Realoca arrays se capacidade insuficiente (dobra a capacidade)
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
            // Armazena os valores nos arrays
            X[N] = x;
            Y[N] = y;
            N++;  // Incrementa contador de pontos
        }
    }
    fclose(arquivo);  // Fecha arquivo após leitura completa

    // ==================== PREPARAÇÃO PARA PROCESSAMENTO PARALELO ====================
    // Aloca array para resultados parciais de cada thread
    parciais = malloc(numThreads * sizeof(Parcial));
    if (!parciais) {
        fprintf(stderr, "Erro ao alocar parciais\n");
        free(X); free(Y);
        return 1;
    }
    
    
    // ==================== CRIAÇÃO E EXECUÇÃO DAS THREADS (PRIMEIRA FASE) ====================
    // PRIMEIRA FASE: cálculo das somas para regressão linear
    pthread_t threads[numThreads];  // Array para armazenar identificadores das threads

    GET_TIME(inicio);  // Inicia medição do tempo dos CÁLCULOS PARALELOS (regressão)
    
    // Cria todas as threads para calcular somas da regressão
    for (long t = 0; t < numThreads; t++) {
        // Cria thread que executará calcula_somas com argumento t (ID da thread)
        pthread_create(&threads[t], NULL, calcula_somas, (void *)t);
    }

    // Aguarda término de todas as threads (sincronização)
    for (int t = 0; t < numThreads; t++) {
        pthread_join(threads[t], NULL);
    }

    // ==================== REDUÇÃO DOS RESULTADOS PARCIAIS (REGRESSÃO) ====================
    // Combina resultados de todas as threads em somas globais
    double somaX = 0, somaY = 0, somaX2 = 0, somaXY = 0;
    for (int t = 0; t < numThreads; t++) {
        somaX  += parciais[t].somaX;   // Soma global de X
        somaY  += parciais[t].somaY;   // Soma global de Y
        somaX2 += parciais[t].somaX2;  // Soma global de X²
        somaXY += parciais[t].somaXY;  // Soma global de X*Y
    }

    // ==================== CÁLCULO DOS COEFICIENTES DA REGRESSÃO ====================
    // Fórmula do coeficiente angular B: B = (n*Σxy - Σx*Σy) / (n*Σx² - (Σx)²)
    double B = (N * somaXY - somaX * somaY) / (N * somaX2 - somaX * somaX);
    
    // Fórmula do coeficiente linear A: A = (Σy - B*Σx) / n
    double A = (somaY - B * somaX) / N;

    GET_TIME(fim);  // Fim da medição dos cálculos da regressão

    // ==================== SEGUNDA FASE: CÁLCULO DO MSE EM PARALELO ====================

    ArgsMSE args_mse[numThreads];  // Array de estruturas de argumentos
    pthread_t threads_mse[numThreads];  // Threads específicas para MSE

    // Prepara argumentos e cria threads para MSE
    for (long t = 0; t < numThreads; t++) {
        long base = N / numThreads;
        long resto = N % numThreads;
        long inicio = t * base + (t < resto ? t : resto);
        long fim = inicio + base + (t < resto ? 1 : 0);
    
        args_mse[t].id = t;
        args_mse[t].A = A;  // Passa coeficiente A
        args_mse[t].B = B;  // Passa coeficiente B  
        args_mse[t].inicio = inicio;
        args_mse[t].fim = fim;
    
        pthread_create(&threads_mse[t], NULL, calcula_mse, &args_mse[t]);
}

    // Aguarda threads do MSE
    for (int t = 0; t < numThreads; t++) {
        pthread_join(threads_mse[t], NULL);
    }

    // ==================== REDUÇÃO DOS RESULTADOS PARCIAIS (MSE) ====================
    // Combina resultados do erro quadrático de todas as threads
    double somaErroQuadTotal = 0;
    for (int t = 0; t < numThreads; t++) {
        somaErroQuadTotal += parciais[t].somaErroQuad;
    }

    // ==================== CÁLCULO FINAL DO MSE ====================
    // MSE = (1/n) * Σ(y - ŷ)²
    double MSE = somaErroQuadTotal / N;

    GET_TIME(fim);  // Fim da medição dos cálculos da regressão
    GET_TIME(fim_total);  // Fim da medição do tempo TOTAL

    // ==================== EXIBIÇÃO DOS RESULTADOS ====================
    printf("\n=== RESULTADOS ===\n");
    printf("Numero de pontos: %ld\n", N);
    printf("Threads usadas: %d\n", numThreads);
    printf("A (intercepto): %.6f\n", A);  // Coeficiente linear (intercepto y)
    printf("B (inclinacao): %.6f\n", B);  // Coeficiente angular (inclinação)
    printf("MSE (Erro Quadratico Medio): %.6f\n", MSE);  // MSE ADICIONADO

    printf("\n=== TEMPOS DE EXECUCAO ===\n");
    printf("Tempo regressao: %f segundos\n", fim - inicio);        // Tempo da regressão linear
    printf("Tempo total programa: %f segundos\n", fim_total - inicio_total); // Programa completo

    // ==================== MODO INTERATIVO DE PREVISÃO ====================
    prever_valores(A, B);

    // ==================== LIMPEZA DE MEMÓRIA ====================
    free(X);        // Libera array de valores X
    free(Y);        // Libera array de valores Y  
    free(parciais); // Libera array de resultados parciais
    
    return 0;
}