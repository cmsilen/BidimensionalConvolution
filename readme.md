## COMANDI ##
### Compilazione
comando per compilare mainMultiThreadV\<number\>.c
```bash
./compile <number>
```
l'output sarà sempre main.exe

### Test del tempo di esecuzione
test per calcolare tempo di esecuzione, throughput e speedup
```bash
./test <version>
```
effettua 5 test per ogni configurazione di thread e carico di lavoro. Il filtro è fissato 7x7.

### Test scalabilità
test per ricavare il tempo di esecuzione al variare della dimensione del filtro
```bash
./scalability <version>
```
effettua 5 test per ogni configurazione di filtro. I threads sono fissati a 16 ed il carico di lavoro alto.

## OTTIMIZZAZIONI #
### OTTIMIZZAZIONE V1 -> V4
```c++
int16_t applyFilter(int16_t** matrix, uint16_t x, uint16_t y, double** filter) {
    double result = 0;
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
```
Si evita di ricalcolare gli indici per più volte per ogni iterazione e rimossi gli if dentro i loop.
___
```c++
double fast_exp(double x) {
    const int k = 40; // x/k ∈ [0, 5] anche se x = 200
    double z = x / k;

    // Padé(3,3)
    double z2 = z * z;
    double z3 = z2 * z;
    double num = 1.0 - z + 0.5 * z2 - z3 / 6.0;
    double den = 1.0 + z + 0.5 * z2 + z3 / 6.0;
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
```
approssimazione dell'esponenziale.
___
```c++
DWORD WINAPI threadFun(LPVOID lpParam) {
    uint16_t i, j, k;
    struct parameters* params = (struct parameters*)lpParam;

    double** filter = initializedoubleMatrix(ROWS_FILTER, COLUMNS_FILTER);

    for(i = 0; i < LAYERS_NUM; i++) {
        for(j = params->startIndex; j < params->endIndex; j++) {
            for(k = 0; k < COLUMNS_MATRIX; k++) {
                if(depthMap[j][k] == 0) {
                    results[i][j][k] = matrices[i][j][k];
                    continue;
                }
                computeFilter(filter, j, k);
                results[i][j][k] = applyFilter(matrices[i], j, k, filter);
            }
        }
    }

    free(filter);
    return 0;
}
```
Evita il calcolo del filtro per i pixel in cui depthmap = 0
___
### OTTIMIZZAZIONE V4 -> V6
assegnazione di righe ai threads al posto delle colonne
___
```bash
gcc -g3 -O3 -ffast-math -march=native -fno-omit-frame-pointer -m64 -o main %FILE%
```
ottimizzazioni aggressive del compilatore attive (-O3),\
ottimizzazione matematiche aggressive (-ffast-math),\
ottimizzazioni personalizzate per il tipo di processore (-march=native)
___
```c++
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
                if(depthMap[currentRow][k] == 0) {
                    results[i][currentRow][k] = matrices[i][currentRow][k];
                    continue;
                }
                computeFilter(filter, currentRow, k);
                results[i][currentRow][k] = applyFilter(matrices[i], currentRow, k, filter);
            }
        }
    }

    free(filter);
    return 0;
}
```
load balancing tra threads.
