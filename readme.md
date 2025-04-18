## COMPILARE ##
### Compilazione dell'ultima versione
comando per compilare mainMultiThreadV\<number\>.c
```bash
./compile <number>
```
l'output sarà sempre main.exe

## OTTIMIZZAZIONI V2 ##

```c++
int16_t applyFilter(int16_t** matrix, uint16_t x, uint16_t y, int16_t** filter) {
    int16_t result = 0;
    uint16_t i, j;

    for (i = 0; i < ROWS_FILTER; i++) {
        int k = x - 1 + i;
        if (k < 0 || k >= ROWS_MATRIX)
            continue;

        for (j = 0; j < COLUMNS_FILTER; j++) {
            int h = y - 1 + j;
            if (h < 0 || h >= COLUMNS_MATRIX)
                continue;
            result += matrix[k][h] * filter[i][j];
        }
    }
    return result;
}
```
Si evita di ricalcolare gli indici k ed h ogni volta che li si utilizzano.

## OTTIMIZZAZIONI V3 ##
```c++
int16_t applyFilter(int16_t** matrix, uint16_t x, uint16_t y, int16_t** filter) {
int16_t result = 0;
uint16_t i, j;

    uint16_t startX = 0;
    uint16_t startY = 0;
    if(x == 0) startX = 1;
    if(y == 0) startY = 1;

    uint16_t endX = ROWS_FILTER;
    uint16_t endY = COLUMNS_FILTER;
    if(x == ROWS_MATRIX - 1) endX = ROWS_FILTER - 1;
    if(y == COLUMNS_MATRIX - 1) endY = COLUMNS_FILTER - 1;

    int k = x - 1 + startX;
    for (i = startX; i < endX; i++) {
    	int h = y - 1 + startY;
        for (j = startY; j < endY; j++) {
            result += matrix[k][h] * filter[i][j];
            h++;
        }
        k++;
    }
    return result;
}
```
Rimossi gli if dentro i loop.