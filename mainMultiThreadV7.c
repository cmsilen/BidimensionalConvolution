#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>
#include <locale.h>

#define SIGMA_MAX 0.01
#define ROWS_MATRIX 2160
#define COLUMNS_MATRIX 1440
#define MAX_NUMBER 255
#define MIN_NUMBER 0
#define MIN(a,b) (((a)<(b))?(a):(b))

#define DEBUG 0

uint16_t ROWS_FILTER;
uint16_t COLUMNS_FILTER;
uint16_t LAYERS_NUM;
uint16_t HALF_ROW;
uint16_t HALF_COLUMN;

HANDLE hMutex;

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

double** initializeDoubleMatrix(uint16_t rows, uint16_t cols) {
    uint16_t i, j = 0;
    double** matrix;

    if (rows == 0 || cols == 0) {
        return 0;
    }

    matrix = malloc(sizeof(double*) * rows);
    for(i = 0; i < rows; i++) {
        matrix[i] = malloc(sizeof(double) * cols);
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

double fast_exp(double x) {
    const int k = 40; // x/k ∈ [0, 5] anche se x = 200
    double z = x * 0.025;

    // Padé(3,3)
    double z2 = z * z;
    double z3 = z2 * z;
    double num = 1.0 - z + 0.5 * z2 - z3 * 0.16666667;
    double den = 1.0 + z + 0.5 * z2 + z3 * 0.16666667;
    double base = num / den;

    // Esponenziazione rapida base^k
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

double fast_sqrt(double x) {
    float x_half = 0.5f * x;
    int i = *(int*)&x;           // interpret float as int
    i = 0x5f3759df - (i >> 1);   // magic number (inverso sqrt)
    x = *(float*)&i;
    x = x * (1.5f - x_half * x * x);  // Newton-Raphson
    return 1.0f / x;
}
// ---------------------------------------------------------- //

int16_t** depthMap;

// depends on sigma and the coords of the filter
double gaussianBlur(uint16_t i, uint16_t j, double sigma) {
    double sigmaSq2 = sigma * sigma * 2;
    double denominator = fast_sqrt(sigmaSq2 * 3.14);

    double it = i - HALF_ROW;
    double jt = j - HALF_COLUMN;

    double exponent = (it * it + jt * jt) / sigmaSq2;
    return (1.0 / denominator) * fast_exp(exponent);
}

// to compute the filter given the coords of the matrix
void computeFilter(double** filter, uint16_t row, uint16_t col) {
    double sigma = (depthMap[row][col] == 0 ? 1 : depthMap[row][col]) * SIGMA_MAX;
    for (uint16_t i = 0; i < ROWS_FILTER; i++) {
        for (uint16_t j = 0; j < COLUMNS_FILTER; j++) {
            filter[i][j] = gaussianBlur(i, j, sigma);
        }
    }
}

int16_t applyFilter(int16_t** matrix, uint16_t x, uint16_t y, double** filter) {
    double result = 0;
    uint16_t i, j;

    uint16_t startX = 0;
    uint16_t startY = 0;
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

uint16_t lastRow = 0;


DWORD WINAPI threadFun(LPVOID lpParam) {
    uint16_t i, j, k;

    double** filter = initializeDoubleMatrix(ROWS_FILTER, COLUMNS_FILTER);
    uint16_t currentRow;

    while(1) {
        // Entra nella sezione critica
        WaitForSingleObject(hMutex, INFINITE);

        if (lastRow >= ROWS_MATRIX) {
            // Esci se il limite è stato raggiunto
            ReleaseMutex(hMutex);
            break;
        }
        currentRow = lastRow;
        lastRow++;
        ReleaseMutex(hMutex);

        for(i = 0; i < LAYERS_NUM; i++) {
            for(k = 0; k < COLUMNS_MATRIX; k++) {
                computeFilter(filter, currentRow, k);
                results[i][currentRow][k] = applyFilter(matrices[i], currentRow, k, filter);
            }
        }
    }

    free(filter);
    return 0;
}

double experiment(uint8_t nThreads, uint8_t debug) {
    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq);

    uint16_t i;
    HANDLE threads[nThreads];
    struct parameters* params[nThreads];

    // Crea il mutex
    hMutex = CreateMutex(NULL, FALSE, NULL);
    if (hMutex == NULL) {
        printf("Errore nella creazione del mutex\n");
        return 1;
    }


    // computation phase
    if(debug) {
        printf("\n");
        printf("\n");
        printf("starting computations\n");
    }

    QueryPerformanceCounter(&start);
    for(i = 0; i < nThreads; i++) {
        threads[i] = CreateThread(NULL, 0, threadFun, NULL, 0, NULL);
        //SetThreadPriority(threads[i], THREAD_PRIORITY_TIME_CRITICAL);
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

    HALF_ROW = ROWS_FILTER / 2;
    HALF_COLUMN = COLUMNS_FILTER / 2;

    uint8_t i;
    matrices = malloc(sizeof(int16_t**) * LAYERS_NUM);
    results = malloc(sizeof(int16_t**) * LAYERS_NUM);
    depthMap = generateRandomMatrix(ROWS_MATRIX, COLUMNS_MATRIX);

    // generation phase
    for(i = 0; i < LAYERS_NUM; i++) {
        matrices[i] = generateRandomMatrix(ROWS_MATRIX, COLUMNS_MATRIX);
        results[i] = initializeMatrix(ROWS_MATRIX, COLUMNS_MATRIX);
    }

    double resultExTime = experiment(NThread, DEBUG);

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
		FILE* file = fopen("resultsV7/scalability.csv", "r");
	    int exists = file != NULL;
	    fclose(file);
    	char filename[100] = "resultsV7/scalability.csv";
    	file = fopen(filename, "a");

	    if(exists == 0) {
	        fprintf(file, "RowsFilter;executionTime\n");
	    }

    	fprintf(file, "%u;%.3f\n", ROWS_FILTER, resultExTime);
    	fclose(file);
		return 0;
	}

    char filename[100] = "resultsV7/executionTime_";
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
