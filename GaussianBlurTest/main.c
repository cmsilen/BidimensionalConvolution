#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>
#include <locale.h>

#define SIGMA_S 4.0
#define SIGMA_D 20.0

#define ROWS_MATRIX 512
#define COLUMNS_MATRIX 512

uint16_t ROWS_FILTER = 13;
uint16_t COLUMNS_FILTER = 13;

int16_t** img;
int16_t** depth;
int16_t** out_img;

int16_t** initializeMatrix(uint16_t rows, uint16_t cols);
int read_pgm(const char* filename, int16_t** img, int* width, int* height);
int write_pgm(const char* filename, int16_t** out, int width, int height);

double gaussianBlur(uint16_t i, uint16_t j, double sigma);
double sigmaFunction(uint16_t i, uint16_t j);
void computeFilter(int16_t** filter, uint16_t row, uint16_t col);
int16_t applyFilter(int16_t** matrix, uint16_t y, uint16_t x);

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

    for(int j = 0; j < 512; j++) {
        for(int k = 0; k < 512; k++) {
            out_img[k][j] = applyFilter(img, k, j);
        }
    }

    write_pgm("out_img.pgm", out_img, 512, 512);
}

int16_t applyFilter(int16_t** matrix, uint16_t y, uint16_t x) {
    double num = 0.0, den = 0.0;

    int HALF_FILTER_SIZE = ROWS_FILTER/2;

    for (int16_t i = -HALF_FILTER_SIZE; i <= HALF_FILTER_SIZE; i++) {
        for (int16_t j = -HALF_FILTER_SIZE; j <= HALF_FILTER_SIZE; j++) {
            int32_t ny = y + i;
            int32_t nx = x + j;

            if (ny >= 0 && ny < ROWS_MATRIX && nx >= 0 && nx < COLUMNS_MATRIX) {
                uint32_t spatial_dist2 = i*i + j*j;
                int16_t depth_diff = depth[ny][nx] - depth[y][x];
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
        return (int16_t)(result > 255 ? 255 : result);
    }

    return matrix[y][x];
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

int16_t** initializeMatrix(uint16_t rows, uint16_t cols) {
    uint16_t i, j = 0;
    int16_t** matrix;

    if (rows == 0 || cols == 0) {
        return 0;
    }

    matrix = malloc(sizeof(int16_t*) * rows);
    for(i = 0; i < rows; i++) {
        matrix[i] = malloc(sizeof(int16_t*) * cols);
        for(j = 0; j < cols; j++) {
            matrix[i][j] = 0;
        }
    }
    return matrix;
}
