#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define ROWS_MATRIX 3840
#define COLUMNS_MATRIX 2160
#define LAYERS_NUM 100
#define ROWS_FILTER 3
#define COLUMNS_FILTER 3
#define MAX_NUMBER 5
#define MIN_NUMBER -5

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

    matrix = malloc(sizeof(uint16_t*) * rows);
    for(i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    return;
}

int16_t randomNumber(int16_t min, int16_t max) {
    return rand() % (max - min + 1) + min;
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

int main(void) {
    uint16_t i;
    int16_t*** matrices = malloc(sizeof(int16_t**) * LAYERS_NUM);
    int16_t*** filters = malloc(sizeof(int16_t**) * LAYERS_NUM);
    
    int16_t** result;

    // generation phase
    printf("generating layers\n");
    for(i = 0; i < LAYERS_NUM; i++) {
        matrices[i] = generateRandomMatrix(ROWS_MATRIX, COLUMNS_MATRIX);
        filters[i] = generateRandomMatrix(ROWS_FILTER, COLUMNS_FILTER);
    }

    // computation phase
    printf("starting computations\n");
    for(i = 0; i < LAYERS_NUM; i++) {
        result = bidimensionalConvolution(matrices[i], filters[i]);
    }
    printf("ended computations\n");

    // releasing memory
    for(i = 0; i < LAYERS_NUM; i++) {
        uninitializeMatrix(matrices[i], ROWS_MATRIX, COLUMNS_MATRIX);
        uninitializeMatrix(filters[i], ROWS_FILTER, COLUMNS_FILTER);
    }

    //printMatrix(result, ROWS_MATRIX, COLUMNS_MATRIX);
    return 0;
}
