#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>
#include <locale.h>

#define SIGMA_MAX 0.5
#define ROWS_MATRIX 2160
#define COLUMNS_MATRIX 1440
#define MAX_NUMBER 5
#define MIN_NUMBER -5
#define MIN(a,b) (((a)<(b))?(a):(b))

#define DEBUG 0

uint16_t ROWS_FILTER;
uint16_t COLUMNS_FILTER;
uint16_t LAYERS_NUM;


// ------------------------ UTILITY ------------------------ //
int16_t** initializeMatrix(uint16_t rows, uint16_t cols) {
    uint16_t i, j = 0;
    int16_t** matrix;

    if (rows == 0 || cols == 0) {
        return 0;
    }

    matrix = malloc(sizeof(int16_t*) * rows);
    for(i = 0; i < rows; i++) {
        matrix[i] = malloc(sizeof(int16_t) * cols);
        for(j = 0; j < cols; j++) {
            matrix[i][j] = 0;
        }
    }
    return matrix;
}

float** initializefloatMatrix(uint16_t rows, uint16_t cols) {
    uint16_t i, j = 0;
    float** matrix;

    if (rows == 0 || cols == 0) {
        return 0;
    }

    matrix = malloc(sizeof(float*) * rows);
    for(i = 0; i < rows; i++) {
        matrix[i] = malloc(sizeof(float) * cols);
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
    free(matrix);
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

float fast_exp(float x) {
    const int k = 21;  // exp(-x) = (exp(-x/k))^k
    double z = x / k;

    // Padé(3,3) per exp(-z)
    double z2 = z * z;
    double z3 = z2 * z;
    double num = 1.0 - z + 0.5 * z2 - z3 / 6.0;
    double den = 1.0 + z + 0.5 * z2 + z3 / 6.0;
    double base = num / den;

    // Esponenziazione rapida: base^k
    double result = 1.0;
    double p = base;
    int n = k;

    while (n > 0) {
        if (n & 1) result *= p;
        p *= p;
        n >>= 1;
    }

    return result;
}
// ---------------------------------------------------------- //

int16_t** depthMap;

// depends on sigma and the coords of the filter
float gaussianBlur(uint16_t i, uint16_t j, float sigma) {
    float denominator = sqrt(2 * 3.14 * sigma * sigma);

    float it = i - ROWS_FILTER / 2;
    float jt = j - COLUMNS_FILTER / 2;

    float exponent = (it * it + jt * jt) / (2 * sigma * sigma);
    return (1.0 / denominator) * fast_exp(exponent);
}

// depends on the coords of the matrix
float sigmaFunction(uint16_t i, uint16_t j) {
    return (depthMap[i][j] == 0 ? 1 : depthMap[i][j]) * SIGMA_MAX;
}

// to compute the filter given the coords of the matrix
void computeFilter(float** filter, uint16_t row, uint16_t col) {
    for (uint16_t i = 0; i < ROWS_FILTER; i++) {
        for (uint16_t j = 0; j < COLUMNS_FILTER; j++) {
            filter[i][j] = gaussianBlur(i, j, sigmaFunction(row, col));
        }
    }
}

int16_t applyFilter(int16_t** matrix, uint16_t x, uint16_t y, float** filter) {
    float result = 0;
    uint16_t i, j;

    uint16_t startX = 0;
    uint16_t startY = 0;
    uint16_t HALF_ROW = (ROWS_FILTER / 2);
    uint16_t HALF_COLUMN = (COLUMNS_FILTER / 2);
    if(x < HALF_ROW) startX = HALF_ROW - x;
    if(y < HALF_COLUMN) startY = HALF_COLUMN - y;

    uint16_t endX = ROWS_FILTER;
    uint16_t endY = COLUMNS_FILTER;
    if(x >= ROWS_MATRIX - HALF_ROW) endX = HALF_ROW + (ROWS_MATRIX - x - 1);
    if(y >= COLUMNS_MATRIX - HALF_COLUMN) endY = HALF_COLUMN + (COLUMNS_MATRIX - y - 1);

    uint16_t k = x - HALF_ROW + startX;
    for (i = startX; i < endX; i++) {
        uint16_t h = y - HALF_COLUMN + startY;
        for (j = startY; j < endY; j++) {
            result += matrix[k][h] * filter[i][j];
            h++;
        }
        k++;
    }

    if (result > 255)
        return 255;
    return result;
}

struct parameters {
    uint16_t startIndex;
    uint16_t endIndex;
};

int16_t*** matrices;
int16_t*** results;


DWORD WINAPI threadFun(LPVOID lpParam) {
    uint16_t i, j, k;
    struct parameters* params = (struct parameters*)lpParam;

    float** filter = initializefloatMatrix(ROWS_FILTER, COLUMNS_FILTER);

    for(i = 0; i < LAYERS_NUM; i++) {
        for(j = params->startIndex; j < params->endIndex; j++) {
            for(k = 0; k < COLUMNS_MATRIX; k++) {
                computeFilter(filter, j, k);
                results[i][j][k] = applyFilter(matrices[i], j, k, filter);
            }
        }
    }

    free(filter);
    return 0;
}

float experiment(uint8_t nThreads, uint8_t debug) {
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

    float elapsedTime = (float)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000.0;
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
    // Verifica che siano stati forniti due argomenti
    if (argc != 6) {
        printf("./main <N_THREADS> <N_IMGS> <ROWS_FILTER> <isScalability> <saveData>\n");
        return 1; // Esce con codice di errore
    }

    // Converte gli argomenti in interi
    int NThread = atoi(argv[1]);
    int NImgs = atoi(argv[2]);
	ROWS_FILTER = atoi(argv[3]);
	COLUMNS_FILTER = ROWS_FILTER;
	int isScalability = atoi(argv[4]);
    LAYERS_NUM = NImgs * 3;
    int saveData = atoi(argv[5]);

    uint8_t i;
    matrices = malloc(sizeof(int16_t**) * LAYERS_NUM);
    results = malloc(sizeof(int16_t**) * LAYERS_NUM);
    depthMap = generateRandomMatrix(ROWS_MATRIX, COLUMNS_MATRIX);

    // generation phase
    for(i = 0; i < LAYERS_NUM; i++) {
        matrices[i] = generateRandomMatrix(ROWS_MATRIX, COLUMNS_MATRIX);
        results[i] = initializeMatrix(ROWS_MATRIX, COLUMNS_MATRIX);
    }

    float resultExTime = experiment(NThread, DEBUG);

    printf("%d threads, %d imgs, %u filter: %.3f ms\n", NThread, NImgs, ROWS_FILTER, resultExTime);

    // releasing memory
    uninitializeMatrix(depthMap, ROWS_MATRIX, COLUMNS_MATRIX);
    for(i = 0; i < LAYERS_NUM; i++) {
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
		FILE* file = fopen("resultsV3/scalability.csv", "r");
	    int exists = file != NULL;
	    fclose(file);
    	char filename[100] = "resultsV3/scalability.csv";
    	file = fopen(filename, "a");

	    if(exists == 0) {
	        fprintf(file, "RowsFilter;executionTime\n");
	    }

    	fprintf(file, "%u;%.3f\n", ROWS_FILTER, resultExTime);
    	fclose(file);
		return 0;
	}

    char filename[100] = "resultsV3/executionTime_";
    concatStringNumber(filename, NImgs);
    strcat(filename, "IMGS.csv\0");
    FILE* file = fopen(filename, "r");
    int exists = file != NULL;
    fclose(file);

    file = fopen(filename, "a");

    if(exists == 0) {
        fprintf(file, "Threads;NImgs;RowsFilter;executionTime\n");
    }

    fprintf(file, "%d;%d;%d;%.3f\n", NThread, NImgs, ROWS_FILTER, resultExTime);
    fclose(file);
    return 0;
}
