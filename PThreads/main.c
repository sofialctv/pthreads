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
#define NUM_THREADS 4
#define SEMENTE 321
#define BLOCO_LINHAS 1000
#define BLOCO_COLUNAS 1000


// Matriz global e variável global para contagem de primos
int** matriz;
int quantidade_primos = 0;
pthread_mutex_t mutex_contador;

// Calcula o número total de blocos na matriz
const int num_blocos_linha = MATRIZ_LINHAS / BLOCO_LINHAS;
const int num_blocos_coluna = MATRIZ_COLUNAS / BLOCO_COLUNAS;
int total_blocos;

// Declaração da variável bloco_status globalmente
int* bloco_status;

// Função para verificar se um número é primo
int ehPrimo(int n) {
    if (n <= 1) return 0; // se o numero for menor ou igual a 1 então nao é primo.
    double limite = sqrt(n);
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

// Função para inicializar a matriz com números aleatórios
void criarMatriz() {
    matriz = (int**)malloc(MATRIZ_LINHAS * sizeof(int*));
    for (int i = 0; i < MATRIZ_LINHAS; i++) {
        matriz[i] = (int*)malloc(MATRIZ_COLUNAS * sizeof(int));
        for (int j = 0; j < MATRIZ_COLUNAS; j++) {
            matriz[i][j] = rand() % 32000; // Números aleatórios no intervalo 0 a 31999
        }
    }
}

// Função para buscar primos serialmente
void buscaSerial() {
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
}

// Função que cada thread executa
void* trabalhoThread(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex_contador); // Bloquear o mutex antes de acessar block_status
        int bloco_atual = -1;
        for (int i = 0; i < total_blocos; i++) {
            if (bloco_status[i] == 0) {
                bloco_status[i] = 1;
                bloco_atual = i;
                break;
            }
        }
        pthread_mutex_unlock(&mutex_contador); // Desbloquear o mutex após acessar block_status

        if (bloco_atual == -1) break; // Todos os macroblocos já foram processados

        // Calcular o intervalo de linhas que esta thread deve verificar
        int start_row = (bloco_atual / num_blocos_coluna) * BLOCO_LINHAS;
        int end_row = start_row + BLOCO_LINHAS;

        // Calcular o intervalo de colunas
        int start_col = (bloco_atual % num_blocos_coluna) * BLOCO_COLUNAS;
        int end_col = start_col + BLOCO_COLUNAS;

        // Verificar os números primos no intervalo de linhas e colunas atribuído
        for (int i = start_row; i < end_row; i++) {
            for (int j = start_col; j < end_col; j++) {
                if (ehPrimo(matriz[i][j])) {
                    pthread_mutex_lock(&mutex_contador); // Bloquear o mutex antes de acessar prime_count
                    quantidade_primos++;
                    pthread_mutex_unlock(&mutex_contador); // Desbloquear o mutex após acessar prime_count
                }
            }
        }
    }

    pthread_exit(NULL);
}

// Função para buscar primos paralelamente
void buscaParalela() {
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
    printf("Quantidade de primos: %d\n", quantidade_primos);
    printf("Tempo decorrido: %f segundos\n", tempo_total);
}

int main() {
    // Inicializar mutex
    pthread_mutex_init(&mutex_contador, NULL);

    // Inicializar matriz
    srand(SEMENTE); // Semente pré-definida
    criarMatriz();

    // Buscar primos serialmente
    buscaSerial();

    // Reiniciar contagem de primos
    quantidade_primos = 0;

    // Calcula a quantidade de blocos na matriz
    total_blocos = num_blocos_linha * num_blocos_coluna;

    // Aloca memória para bloco_status
    bloco_status = (int*)malloc(total_blocos * sizeof(int));

    // Inicializa bloco_status
    for (int i = 0; i < total_blocos; i++) {
        bloco_status[i] = 0;
    }

    printf("\nNumero de blocos na matriz: %d\n\n", total_blocos);

    // Buscar primos paralelamente
    buscaParalela();

    // Liberar memória
    for (int i = 0; i < MATRIZ_LINHAS; i++) {
        free(matriz[i]);
    }
    free(matriz);

    // Destruir mutex
    pthread_mutex_destroy(&mutex_contador);

    return 0;
}
