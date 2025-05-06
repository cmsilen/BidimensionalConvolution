#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>
#include <locale.h>

#define SIGMA_MAX 0.1
#define ROWS_MATRIX 512
#define COLUMNS_MATRIX 512

uint16_t ROWS_FILTER = 13;
uint16_t COLUMNS_FILTER = 13;

int16_t** img;
int16_t** depth;
int16_t** out_img;

double** initializeMatrix(uint16_t rows, uint16_t cols);
int read_pgm(const char* filename, int16_t** img, int* width, int* height);
int write_pgm(const char* filename, int16_t** out, int width, int height);

double gaussianBlur(uint16_t i, uint16_t j, double sigma);
double sigmaFunction(uint16_t i, uint16_t j);
void computeFilter(double** filter, uint16_t row, uint16_t col);
int16_t applyFilter(int16_t** matrix, uint16_t y, uint16_t x, double** filter);

int main(int argc, char *argv[]) 
{
    int width, height;

    // Alloca la memoria per l'immagine di input/output e depth map
    img = malloc(sizeof(int16_t*) * 512);
    depth = malloc(sizeof(int16_t*) * 512);
    out_img = malloc(sizeof(int16_t*) * 512);
    for (int i = 0; i < 512; i++) {
        img[i] = malloc(sizeof(int16_t) * 512);
        depth[i] = malloc(sizeof(int16_t) * 512);
        out_img[i] = malloc(sizeof(int16_t) * 512);
    }

    if (read_pgm("input_img.pgm", img, &width, &height) != 0) {
        printf("Errore lettura dell'immagine: %dx%d\n", width, height);
        return -1;
    }

    if (read_pgm("input_depth.pgm", depth, &width, &height) != 0) {
        printf("Errore lettura della depth map: %dx%d\n", width, height);
        return -1;
    }

    double** filter = initializeMatrix(ROWS_FILTER, COLUMNS_FILTER);

    for(int j = 0; j < 512; j++) {
        for(int k = 0; k < 512; k++) {
            computeFilter(filter, k, j);
            out_img[k][j] = applyFilter(img, k, j, filter);
        }
    }

    write_pgm("out_img.pgm", out_img, 512, 512);
}


// depends on sigma and the coords of the filter
double gaussianBlur(uint16_t i, uint16_t j, double sigma) {
    double denominator = sqrt(2 * 3.14 * sigma * sigma);
    double exponent = -(i * i + j * j) / (2 * sigma * sigma);
    return (1.0 / denominator) * exp(exponent);
}

// depends on the coords of the matrix
double sigmaFunction(uint16_t i, uint16_t j) {
    return depth[i][j] * SIGMA_MAX;
}

// to compute the filter given the coords of the matrix
void computeFilter(double** filter, uint16_t row, uint16_t col) {
    for (uint16_t i = 0; i < ROWS_FILTER; i++) {
        for (uint16_t j = 0; j < COLUMNS_FILTER; j++) {
            filter[i][j] = gaussianBlur(i, j, sigmaFunction(row, col));
        }
    }
}

int16_t applyFilter(int16_t** matrix, uint16_t x, uint16_t y, double** filter) {
    int16_t result = 0;
    uint16_t i, j;

    for (i = 0; i < ROWS_FILTER; i++) {
        for (j = 0; j < COLUMNS_FILTER; j++) {
            if (x - (ROWS_FILTER / 2) + i < 0 || x - (ROWS_FILTER / 2) + i >= ROWS_MATRIX ||
                y - (COLUMNS_FILTER / 2) + j < 0 || y - (COLUMNS_FILTER / 2) + j >= COLUMNS_MATRIX)
                continue;
            result += matrix[x - (ROWS_FILTER / 2) + i][y - (COLUMNS_FILTER / 2) + j] * filter[i][j];
        }
    }
    return result;
}

int read_pgm(const char* filename, int16_t** img, int* width, int* height) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Errore nell'apertura del file");
        return -1;
    }

    char line[256];
    
    // Legge il tipo PGM (deve essere "P2")
    if (!fgets(line, sizeof(line), file)) {
        fclose(file);
        return -1;
    }
    if (strncmp(line, "P2", 2) != 0) {
        fprintf(stderr, "Formato non supportato (non P2)\n");
        fclose(file);
        return -1;
    }

    // Salta commenti e legge le dimensioni
    do {
        if (!fgets(line, sizeof(line), file)) {
            fclose(file);
            return -1;
        }
    } while (line[0] == '#');

    // Legge width e height
    if (sscanf(line, "%d %d", width, height) != 2) {
        fprintf(stderr, "Errore nella lettura delle dimensioni\n");
        fclose(file);
        return -1;
    }

    // Legge il valore massimo di grigio (tipicamente 255)
    int maxval;
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%d", &maxval) != 1) {
        fprintf(stderr, "Errore nella lettura del valore massimo\n");
        fclose(file);
        return -1;
    }

    // Legge i pixel e li salva in img
    for (int y = 0; y < *height; y++) {
        for (int x = 0; x < *width; x++) {
            int pixel;
            if (fscanf(file, "%d", &pixel) != 1) {
                fprintf(stderr, "Errore nella lettura dei pixel\n");
                fclose(file);
                return -1;
            }
            img[y][x] = (int16_t)pixel;
        }
    }

    fclose(file);
    return 0;
}

int write_pgm(const char* filename, int16_t** out, int width, int height) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Errore nell'apertura del file per la scrittura");
        return -1;
    }

    // Scrive l'header del PGM in formato ASCII
    fprintf(file, "P2\n");
    fprintf(file, "# Generated by write_pgm\n");
    fprintf(file, "%d %d\n", width, height);
    fprintf(file, "255\n");  // Massimo valore di grigio

    // Scrive i pixel riga per riga
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int val = out[y][x];
            if (val < 0) val = 0;
            if (val > 255) val = 255;
            fprintf(file, "%d ", val);
        }
        fprintf(file, "\n");
    }

    fclose(file);
    return 0;
}

double** initializeMatrix(uint16_t rows, uint16_t cols) {
    uint16_t i, j = 0;
    double** matrix;

    if (rows == 0 || cols == 0) {
        return 0;
    }

    matrix = malloc(sizeof(double*) * rows);
    for(i = 0; i < rows; i++) {
        matrix[i] = malloc(sizeof(double*) * cols);
        for(j = 0; j < cols; j++) {
            matrix[i][j] = 0;
        }
    }
    return matrix;
}
