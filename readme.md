## COMPILARE ##
### Compilazione dell'ultima versione
comando per compilare mainMultiThreadV*.c
```bash
./compile
```


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
TODO