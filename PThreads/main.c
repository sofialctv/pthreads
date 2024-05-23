#pragma once
#pragma comment(lib,"pthreadVC2.lib")

#define _CRT_SECURE_NO_WARNINGS 1 
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#define HAVE_STRUCT_TIMESPEC

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#define MATRIZ_LINHAS 10000
#define MATRIZ_COLUNAS 10000
#define NUM_THREADS 8
#define SEMENTE 321
#define BLOCO_LINHAS 100
#define BLOCO_COLUNAS 100
#define NUM_BLOCOS_LINHA (MATRIZ_LINHAS / BLOCO_LINHAS)
#define NUM_BLOCOS_COLUNA (MATRIZ_COLUNAS / BLOCO_COLUNAS)
#define TOTAL_BLOCOS (NUM_BLOCOS_LINHA * NUM_BLOCOS_COLUNA)

// Matriz principal, matriz de controle de blocos e variável global para contagem de primos
int** matriz;
int bloco_status[NUM_BLOCOS_LINHA][NUM_BLOCOS_COLUNA];
int quantidade_primos = 0;

// Mutexes de controle de RC's
pthread_mutex_t mutex_contador;
pthread_mutex_t mutex_bloco_status;

// Função para verificar se um número é primo
int ehPrimo(int n) {
    if (n <= 1) return 0; // se o numero for menor ou igual a 1 então nao é primo.
    double limite = sqrt(n);
    for (int i = 2; i <= limite; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

// Função para inicializar a matriz com números aleatórios
void criarMatriz() {
    matriz = (int**)malloc(MATRIZ_LINHAS * sizeof(int*));

    if (matriz == NULL) {
        printf("Erro ao alocar memoria para matriz");
        exit(1);
    }

    for (int i = 0; i < MATRIZ_LINHAS; i++) {
        matriz[i] = (int*)malloc(MATRIZ_COLUNAS * sizeof(int));

        if (matriz[i] == NULL) {
            printf("Erro ao alocar memoria para matriz");
            exit(1);
        }

        for (int j = 0; j < MATRIZ_COLUNAS; j++) {
            matriz[i][j] = rand() % 32000; // Números aleatórios no intervalo 0 a 31999
        }
    }
}

// Função para buscar primos serialmente
double buscaSerial() {
    clock_t tempo_inicio = clock();

    for (int i = 0; i < MATRIZ_LINHAS; i++) {
        for (int j = 0; j < MATRIZ_COLUNAS; j++) {
            if (ehPrimo(matriz[i][j])) {
                quantidade_primos++;
            }
        }
    }

    clock_t tempo_fim = clock();
    double tempo_total = (double)(tempo_fim - tempo_inicio) / CLOCKS_PER_SEC;

    printf("Busca Serial:\n");
    printf("Quantidade de primos: %d\n", quantidade_primos);
    printf("Tempo decorrido: %f segundos\n", tempo_total);

    return tempo_total;
}

// Função que cada thread executa
void* trabalhoThread(void* arg) {
    int contabilizar_bloco = 0;

    for (int i = 0; i < NUM_BLOCOS_LINHA; i++) {
        for (int j = 0; j < NUM_BLOCOS_COLUNA; j++) {
            contabilizar_bloco = 0;
            pthread_mutex_lock(&mutex_bloco_status);
            if (bloco_status[i][j] == 0) {
                bloco_status[i][j] = 1;
                contabilizar_bloco = 1;
            }
            pthread_mutex_unlock(&mutex_bloco_status);
            if (contabilizar_bloco == 1) {
                int linha_inicio = i * BLOCO_LINHAS;
                int linha_fim = (i + 1) * BLOCO_LINHAS;

                int coluna_inicio = j * BLOCO_COLUNAS;
                int coluna_fim = (j + 1) * BLOCO_COLUNAS;

                for (int i = linha_inicio; i < linha_fim; i++) {
                    for (int j = coluna_inicio; j < coluna_fim; j++) {
                        if (ehPrimo(matriz[i][j])) {
                            pthread_mutex_lock(&mutex_contador);
                            quantidade_primos++;
                            pthread_mutex_unlock(&mutex_contador);
                        }

                    }
                }
            }

        }
    }

    pthread_exit(NULL);
}

// Função para buscar primos paralelamente
double buscaParalela() {
    clock_t tempo_inicio = clock();

    pthread_t threads[NUM_THREADS];

    // Criar e executar threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, trabalhoThread, NULL);
    }

    // Aguardar as threads terminarem
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_t tempo_fim = clock();
    double tempo_total = (double)(tempo_fim - tempo_inicio) / CLOCKS_PER_SEC;

    printf("Busca Paralela:\n");
    printf("Quantidade de threads: %d\n", NUM_THREADS);
    printf("Numero de blocos na matriz: %d\n", TOTAL_BLOCOS);
    printf("Quantidade de primos: %d\n", quantidade_primos);
    printf("Tempo decorrido: %f segundos\n", tempo_total);

    return tempo_total;
}

int main() {
    // Inicializar mutexes
    pthread_mutex_init(&mutex_contador, NULL);
    pthread_mutex_init(&mutex_bloco_status, NULL);

    // Inicializar matriz
    srand(SEMENTE); // Semente pré-definida
    criarMatriz();

    printf("#       Executando Busca Serial     #\n\n");

    // Buscar primos serialmente
    double tempo_serial = buscaSerial();

    // Reiniciar contagem de primos
    quantidade_primos = 0;

    printf("\n#     Executando busca paralela       #\n\n");

    // Iniciando matriz de controle de status dos blocos
    for (int i = 0; i < (NUM_BLOCOS_LINHA); i++) {
        for (int j = 0; j < (NUM_BLOCOS_COLUNA); j++) {
            bloco_status[i][j] = 0; // 0 - Não lido, 1 - Lido
        }
    }

    // Buscar primos paralelamente
    double tempo_paralelo = buscaParalela();

    // Cálculo do speedup
    double speedup = tempo_serial / tempo_paralelo;
    printf("\nSpeedup: %f\n\n\n", speedup);

    // Liberar memória
    for (int i = 0; i < MATRIZ_LINHAS; i++) {
        free(matriz[i]);
    }
    free(matriz);

    // Destruir mutexes
    pthread_mutex_destroy(&mutex_contador);
    pthread_mutex_destroy(&mutex_bloco_status);

    return 0;
    
    system("pause");
}
