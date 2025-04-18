#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#define ROWS_MATRIX 3840
#define COLUMNS_MATRIX 2160
#define ROWS_FILTER 3
#define COLUMNS_FILTER 3
#define MAX_NUMBER 5
#define MIN_NUMBER -5
#define MIN(a,b) (((a)<(b))?(a):(b))

#define N_IMGS 20
#define LAYERS_NUM 3 * N_IMGS    // rgb layers

#define NTHREADS 40
#define DEBUG 0

int16_t** initializeMatrix(uint16_t rows, uint16_t cols) {
    uint16_t i, j = 0;
    int16_t** matrix;
    
    if (rows == 0 || cols == 0) {
        return 0;
    }

    matrix = malloc(sizeof(uint16_t*) * rows);
    for(i = 0; i < rows; i++) {
        matrix[i] = malloc(sizeof(uint16_t*) * cols);
        for(j = 0; j < cols; j++) {
            matrix[i][j] = 0;
        }
    }
    return matrix;
}

void uninitializeMatrix(int16_t** matrix, uint16_t rows, uint16_t cols) {
    uint16_t i, j = 0;
    
    if (rows == 0 || cols == 0) {
        return;
    }

    for(i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    return;
}

int16_t g_seed = 10;
int16_t randomNumber(int16_t min, int16_t max) {
    g_seed = (214013*g_seed+2531011);
    return ((g_seed>>16)&0x7FFF) % (max - min + 1) + min;
    
    //return rand() % (max - min + 1) + min;
}

int16_t** generateRandomMatrix(uint16_t rows, uint16_t cols) {
    uint16_t i, j = 0;
    int16_t** matrix;
    
    if (rows == 0 || cols == 0) {
        return 0;
    }

    matrix = initializeMatrix(rows, cols);
    for(i = 0; i < rows; i++) {
        for(j = 0; j < cols; j++) {
            matrix[i][j] = randomNumber(MIN_NUMBER, MAX_NUMBER);
        }
    }
    return matrix;
}

int16_t applyFilter(int16_t** matrix, uint16_t x, uint16_t y, int16_t** filter) {
    int16_t result = 0;
    uint16_t i, j;

    for (i = 0; i < ROWS_FILTER; i++) {
        for (j = 0; j < COLUMNS_FILTER; j++) {
            if (x - 1 + i < 0 || x - 1 + i >= ROWS_MATRIX || y - 1 + j < 0 || y - 1 + j >= COLUMNS_MATRIX)
                continue;
            result += matrix[x - 1 + i][y - 1 + j] * filter[i][j];
        }
    }
    return result;
}

struct parameters {
    uint16_t startIndex;
    uint16_t endIndex;
};

int16_t*** matrices;
int16_t** filter;
int16_t*** results;


DWORD WINAPI threadFun(LPVOID lpParam) {
    uint16_t i, j, k;
    struct parameters* params = (struct parameters*)lpParam;

    for(i = 0; i < LAYERS_NUM; i++) {
        for(j = params->startIndex; j < params->endIndex; j++) {
            for(k = 0; k < COLUMNS_MATRIX; k++) {
                results[i][j][k] = applyFilter(matrices[i], j, k, filter);
            }
        }
    }
    return 0;
}

double experiment(uint8_t nThreads, uint8_t debug) {
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq);

    uint16_t i;
    HANDLE threads[nThreads];
    struct parameters* params[nThreads];


    // preparing params
    int rowsPerThread = ROWS_MATRIX / nThreads;
    int spareChange = ROWS_MATRIX % nThreads;
    int spareChangePerThread = (spareChange / nThreads) + 1;
    if(debug) {
        printf("assigned %d rows per thread\n", rowsPerThread);
        printf("spare change: %d\n", spareChange);
        printf("spare changePerThread: %d\n", spareChangePerThread);
    }
    int index = 0;
    for(i = 0; i < nThreads; i++) {
        params[i] = malloc(sizeof(struct parameters));
        params[i]->startIndex = index;

        if(spareChange > 0) {
            params[i]->endIndex = index + rowsPerThread + MIN(spareChangePerThread, spareChange);
            spareChange -= MIN(spareChangePerThread, spareChange);
        }
        else {
            params[i]->endIndex = index + rowsPerThread;
        }
        index = params[i]->endIndex;

        if(debug) {
            printf("start: %d, end: %d\trows: %d\n", params[i]->startIndex, params[i]->endIndex, params[i]->endIndex - params[i]->startIndex);
        }
    }


    // computation phase
    if(debug) {
        printf("\n");
        printf("\n");
        printf("starting computations\n");
    }

    QueryPerformanceCounter(&start);
    for(i = 0; i < nThreads; i++) {
        threads[i] = CreateThread(NULL, 0, threadFun, params[i], 0, NULL);
    }
    WaitForMultipleObjects(nThreads, threads, TRUE, INFINITE);
    QueryPerformanceCounter(&end);

    double elapsedTime = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000.0;
    if(debug) {
        printf("ended computations\n");
        printf("execution time: %.3f ms\n", elapsedTime);
    }

    for(i = 0; i < nThreads; i++) {
        CloseHandle(threads[i]);
    }

    return elapsedTime;
}

void concatStringNumber(char *str, int numero) {
    char numStr[20];
    sprintf(numStr, "%d", numero);

    strcat(str, numStr);
}

int main(int argc, char *argv[]) {
    uint8_t i;
    matrices = malloc(sizeof(int16_t**) * LAYERS_NUM);
    filter = generateRandomMatrix(ROWS_FILTER, COLUMNS_FILTER);
    results = malloc(sizeof(int16_t**) * LAYERS_NUM);
    // generation phase
    if(DEBUG) {
        printf("generating layers\n");
    }
    for(i = 0; i < LAYERS_NUM; i++) {
        matrices[i] = generateRandomMatrix(ROWS_MATRIX, COLUMNS_MATRIX);
        results[i] = initializeMatrix(ROWS_MATRIX, COLUMNS_MATRIX);
    }

    double resultExTime[NTHREADS];

    for(i = 1; i <= NTHREADS; i++) {
        resultExTime[i - 1] = experiment(i, DEBUG);
        printf("%d threads: %.3f ms\n", i, resultExTime[i - 1]);
    }

    // releasing memory
    for(i = 0; i < LAYERS_NUM; i++) {
        uninitializeMatrix(matrices[i], ROWS_MATRIX, COLUMNS_MATRIX);
    }
    uninitializeMatrix(filter, ROWS_FILTER, COLUMNS_FILTER);


    FILE* file;
    char filename[100] = "resultsV1/executionTime";
    concatStringNumber(filename, N_IMGS);
    strcat(filename, ".csv\0");
    file = fopen(filename, "w");
    fprintf(file, "nThreads;executionTime\n");

    for(i = 0; i < NTHREADS; i++) {
        fprintf(file, "%d;%.3f\n", i + 1, resultExTime[i]);
    }
    fclose(file);
    return 0;
}
