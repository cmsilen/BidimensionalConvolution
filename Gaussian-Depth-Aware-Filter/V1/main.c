#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>
#include <locale.h>

#define SIGMA_S 4.0
#define SIGMA_D 20.0

#define ROWS_MATRIX 16
#define COLUMNS_MATRIX 16
#define MAX_NUMBER 254
#define MIN_NUMBER 0
#define MIN(a,b) (((a)<(b))?(a):(b))

#define DEBUG 0

uint8_t FILTER_SIZE;
uint8_t HALF_FILTER_SIZE;
uint8_t IMGS_NUM;

// ------------------------ UTILITY ------------------------ //
#pragma region UTILITY
uint8_t** initializeMatrix(uint16_t rows, uint16_t cols)
{
    uint8_t** matrix;
    
    if (rows == 0 || cols == 0) {
        return 0;
    }

    matrix = malloc(sizeof(uint8_t*) * rows);
    for(uint16_t i = 0; i < rows; i++) {
        matrix[i] = malloc(sizeof(uint8_t*) * cols);
        for(uint16_t j = 0; j < cols; j++) {
            matrix[i][j] = 0;
        }
    }
    return matrix;
}

void uninitializeMatrix(uint8_t** matrix, uint16_t rows, uint16_t cols) { 
    if (rows == 0 || cols == 0) {
        return;
    }

    for(uint16_t i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);

    return;
}

uint8_t randomNumber(uint8_t min, uint8_t max) {
    if (min > max) {
        uint8_t temp = min;
        min = max;
        max = temp;
    }

    uint8_t range = max - min + 1;
    if (range <= 255) {
        return (uint8_t)(rand() % range) + min;
    } else {
        return (uint8_t)(rand() % 256);
    }
}

uint8_t** generateRandomMatrix(uint16_t rows, uint16_t cols) {
    uint8_t** matrix;

    if (rows == 0 || cols == 0) {
        return 0;
    }

    matrix = initializeMatrix(rows, cols);
    for(uint16_t i = 0; i < rows; i++) {
        for(uint16_t j = 0; j < cols; j++) {
            matrix[i][j] = randomNumber(MIN_NUMBER, MAX_NUMBER);
        }
    }
    return matrix;
}
#pragma endregion
// ---------------------------------------------------------- //

// depends on sigma and the coords of the filter
uint8_t applyFilter(uint8_t** matrix, uint16_t y, uint16_t x) {
    double num = 0.0, den = 0.0;

    for (int16_t i = -HALF_FILTER_SIZE; i <= HALF_FILTER_SIZE; i++) {
        for (int16_t j = -HALF_FILTER_SIZE; j <= HALF_FILTER_SIZE; j++) {
            int32_t ny = y + i;
            int32_t nx = x + j;

            if (ny >= 0 && ny < ROWS_MATRIX && nx >= 0 && nx < COLUMNS_MATRIX) {
                uint32_t spatial_dist2 = i*i + j*j;
                int16_t depth_diff = matrix[ny][nx] - matrix[y][x];
                uint16_t depth_diff2 = depth_diff * depth_diff;               

                double weight = exp(-(spatial_dist2 / (2.0 * SIGMA_S * SIGMA_S) + depth_diff2 / (2.0 * SIGMA_D * SIGMA_D)));

                //printf("spatial_dist2: %d | depth_diff: %d | depth_diff2: %d | weight: %f\n", spatial_dist2, depth_diff, depth_diff2, weight);

                num += matrix[ny][nx] * weight;
                den += weight;
            }
        }
    }

    if (den > 1e-5) {
        int result = round(num / den);
        return (uint8_t)(result > 255 ? 255 : result);
    }

    return matrix[y][x];
}

struct parameters {
    int startIndex;
    int endIndex;
};

uint8_t*** matrices;
uint8_t*** results;

DWORD WINAPI threadFun(LPVOID lpParam) {
    struct parameters* params = (struct parameters*)lpParam;

    for(uint8_t i = 0; i < IMGS_NUM; i++) 
    {
        for(uint16_t x = params->startIndex; x < params->endIndex; x++) 
        {
            for(uint16_t y = 0; y < ROWS_MATRIX; y++) 
            {
                results[i][y][x] = applyFilter(matrices[i], y, x);
            }
        }
    }

    return 0;
}

double experiment(int nThreads, int debug) {
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq);

    HANDLE threads[nThreads];
    struct parameters* params[nThreads];

    // preparing params
    int colsPerThread = COLUMNS_MATRIX / nThreads;
    int spareChange = COLUMNS_MATRIX % nThreads;
    int spareChangePerThread = (spareChange / nThreads) + 1;
    if(debug) {
        printf("assigned %d cols per thread\n", colsPerThread);
        printf("spare change: %d\n", spareChange);
        printf("spare changePerThread: %d\n", spareChangePerThread);
    }

    uint16_t index = 0;
    for(uint8_t i = 0; i < nThreads; i++) 
    {
        params[i] = malloc(sizeof(struct parameters));
        params[i]->startIndex = index;

        if(spareChange > 0) {
            params[i]->endIndex = index + colsPerThread + MIN(spareChangePerThread, spareChange);
            spareChange -= MIN(spareChangePerThread, spareChange);
        }
        else {
            params[i]->endIndex = index + colsPerThread;
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
    for(uint8_t i = 0; i < nThreads; i++) {
        threads[i] = CreateThread(NULL, 0, threadFun, params[i], 0, NULL);
    }
    WaitForMultipleObjects(nThreads, threads, TRUE, INFINITE);
    QueryPerformanceCounter(&end);

    double elapsedTime = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000.0;
    if(debug) {
        printf("ended computations\n");
        printf("execution time: %.3f ms\n", elapsedTime);
    }

    for(uint8_t i = 0; i < nThreads; i++) {
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
    // Verifica che siano stati forniti due argomenti
    if (argc != 6) {
        printf("./main <N_THREADS> <N_IMGS> <FILTER_SIZE> <isScalability> <saveData>\n");
        return 1; // Esce con codice di errore
    }

    // Converte gli argomenti in interi
    int NThread = atoi(argv[1]);
    IMGS_NUM = atoi(argv[2]);
	FILTER_SIZE = atoi(argv[3]);
	HALF_FILTER_SIZE = FILTER_SIZE / 2;
	int isScalability = atoi(argv[4]);
    int saveData = atoi(argv[5]);

    matrices = malloc(sizeof(uint8_t**) * IMGS_NUM);
    results = malloc(sizeof(uint8_t**) * IMGS_NUM);

    // generation phase
    for(uint8_t i = 0; i < IMGS_NUM; i++) {
        matrices[i] = generateRandomMatrix(ROWS_MATRIX, COLUMNS_MATRIX);
        results[i] = initializeMatrix(ROWS_MATRIX, COLUMNS_MATRIX);
    }

    double resultExTime = experiment(NThread, DEBUG);

    printf("%d threads, %d imgs, %u filter: %.3f ms\n", NThread, IMGS_NUM, FILTER_SIZE, resultExTime);

    // releasing memory
    for(uint8_t i = 0; i < IMGS_NUM; i++) {
        free(matrices[i]);
    }

    if(!saveData) {
        return 0;
    }

    if(setlocale(LC_NUMERIC, "Italian_Italy.1252") == NULL) {
        printf("Failed to set locale\n");
        return 1;
    }

	if(isScalability > 0) {
		FILE* file = fopen("resultsV1/scalability.csv", "r");
	    int exists = file != NULL;
	    fclose(file);
    	char filename[100] = "resultsV1/scalability.csv";
    	file = fopen(filename, "a");

	    if(exists == 0) {
	        fprintf(file, "RowsFilter;executionTime\n");
	    }

    	fprintf(file, "%u;%.3f\n", FILTER_SIZE, resultExTime);
    	fclose(file);
		return 0;
	}

    char filename[100] = "resultsV1/executionTime_";
    concatStringNumber(filename, IMGS_NUM);
    strcat(filename, "IMGS.csv\0");
    FILE* file = fopen(filename, "r");
    int exists = file != NULL;
    fclose(file);

    file = fopen(filename, "a");

    if(exists == 0) {
        fprintf(file, "Threads;NImgs;RowsFilter;executionTime\n");
    }

    fprintf(file, "%d;%d;%d;%.3f\n", NThread, IMGS_NUM, FILTER_SIZE, resultExTime);
    fclose(file);
    return 0;
}
