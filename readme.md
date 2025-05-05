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

## OTTIMIZZAZIONI ##
### OTTIMIZZAZIONE V2
assegnazione di righe ai threads al posto delle colonne

### OTTIMIZZAZIONE V3
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
Si evita di ricalcolare gli indici per più volte per ogni iterazione.

