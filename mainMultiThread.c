#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#define ROWS_MATRIX 1080
#define COLUMNS_MATRIX 720
#define ROWS_FILTER 3
#define COLUMNS_FILTER 3
#define MAX_NUMBER 5
#define MIN_NUMBER -5

#define NTHREADS 1
#define LAYERS_NUM NTHREADS * 10

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

int16_t** bidimensionalConvolution(int16_t** matrix, int16_t** filter) {
    int16_t** result = initializeMatrix(ROWS_MATRIX, COLUMNS_MATRIX);

    for (int i = 0; i < ROWS_MATRIX; i++) {
        for (int j = 0; j < COLUMNS_MATRIX; j++) {
            result[i][j] = applyFilter(matrix, i, j, filter);
        }
    }
    return result;
}

void printMatrix(int16_t** matrix, uint16_t rows, uint16_t cols) {
    uint16_t i, j;
    
    for(i = 0; i < rows; i++) {
        for(j = 0; j < cols; j++) {
            printf("%d\t", matrix[i][j]);
        }
        printf("\n");
    }
}


struct parameters {
    uint8_t startIndex;
    uint8_t endIndex;
};

int16_t*** matrices;
int16_t*** filters;

DWORD WINAPI threadFun(LPVOID lpParam) {
    int16_t** result;
    uint8_t i;
    struct parameters* params = (struct parameters*)lpParam;

    for(i = params->startIndex; i < params->endIndex; i++) {
        result = bidimensionalConvolution(matrices[i], filters[i]);
    }
    return 0;
}

int main(void) {
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq);

    uint16_t i;
    matrices = malloc(sizeof(int16_t**) * LAYERS_NUM);
    filters = malloc(sizeof(int16_t**) * LAYERS_NUM);
    HANDLE threads[NTHREADS];
    struct parameters* params[NTHREADS];

    // generation phase
    printf("generating layers\n");
    for(i = 0; i < LAYERS_NUM; i++) {
        matrices[i] = generateRandomMatrix(ROWS_MATRIX, COLUMNS_MATRIX);
        filters[i] = generateRandomMatrix(ROWS_FILTER, COLUMNS_FILTER);
    }
    // preparing params
    int layersPerThread = LAYERS_NUM / NTHREADS;
    for(i = 0; i < NTHREADS; i++) {
        params[i] = malloc(sizeof(struct parameters));
        params[i]->startIndex = i * layersPerThread;
        params[i]->endIndex = (i + 1) * layersPerThread;
    }


    // computation phase
    printf("starting computations\n");
    QueryPerformanceCounter(&start);
    for(i = 0; i < NTHREADS; i++) {
        threads[i] = CreateThread(NULL, 0, threadFun, params[i], 0, NULL);
    }
    WaitForMultipleObjects(NTHREADS, threads, TRUE, INFINITE);
    QueryPerformanceCounter(&end);
    printf("ended computations\n");
    double elapsedTime = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000.0;
    printf("execution time: %.3f ms\n", elapsedTime);

    for(i = 0; i < NTHREADS; i++) {
        CloseHandle(threads[i]);
    }

    // releasing memory
    for(i = 0; i < LAYERS_NUM; i++) {
        uninitializeMatrix(matrices[i], ROWS_MATRIX, COLUMNS_MATRIX);
        uninitializeMatrix(filters[i], ROWS_FILTER, COLUMNS_FILTER);
    }

    //printMatrix(result, ROWS_MATRIX, COLUMNS_MATRIX);
    return 0;
}
