#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <windows.h>

#define ROWS_TERRAIN 2600
#define COLUMNS_TERRAIN 2600
#define MIN_ALTITUDE 0
#define MAX_ALTITUDE 254
#define N_THREADS 32

#pragma region LIST

typedef struct Node {
    struct Node* next[8];
    uint16_t x;
    uint16_t y;
} Node;

void printList(Node* head) 
{
    if (head == NULL)
        return;

    printf("x: %d | y: %d\n", head->x, head->y);
    for (uint8_t i = 0; i < 8; i++) 
    {
        if (head->next[i] == NULL)
            continue;
        
        printList(head->next[i]);
    }
}

void freeList(Node* head) 
{
    if (head == NULL)
        return;

    for (uint8_t i = 0; i < 8; i++) 
    {
        if (head->next[i] == NULL)
            continue;
        
        freeList(head->next[i]);
    }

    free(head->next);
    free(head);
}

Node* makeNode(uint16_t x, uint16_t y) 
{
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->x = x;
    new_node->y = y;
    memset(new_node->next, 0, sizeof(Node*) * 8);

    return new_node;
}

bool findNode(Node* head, uint16_t x, uint16_t y) 
{
    if (head == NULL)
        return false;
    
    if (head->x == x && head->y == y)
        return true;
    
    for (uint8_t i = 0; i < 8; i++) 
    {
        if (head->next[i] == NULL)
            continue;
        
        if (findNode(head->next[i], x, y))
            return true;
    }

    return false;
}

#pragma endregion

#pragma region Signatures

uint8_t random_uint8(uint8_t min, uint8_t max);
uint8_t* initializeTerrain();
void uninitializeTerrain(uint8_t* terrain);
void printTerrain(uint8_t* terrain);
void printFloodedTerrain(uint8_t* terrain, Node** cellList, uint16_t x, uint16_t y);
void placeSource(uint8_t* terrain, Node** cellList, uint16_t x, uint16_t y);

#pragma endregion

CRITICAL_SECTION cs;
uint32_t last_task_x = 0, last_task_y = 0;
bool getTask(uint32_t* task_x, uint32_t* task_y)
{
    EnterCriticalSection(&cs);

    if (last_task_x == COLUMNS_TERRAIN - 1 && last_task_y == ROWS_TERRAIN - 1) {
        LeaveCriticalSection(&cs);
        return false;
    }

    last_task_x++;
    if (last_task_x == COLUMNS_TERRAIN) {
        last_task_x = 0;
        last_task_y++;
    }

    *task_x = last_task_x;
    *task_y = last_task_y;
    LeaveCriticalSection(&cs);

    return true;
}

uint8_t* terrain;
Node** list;

DWORD WINAPI threadFun(LPVOID lpParam) 
{
    uint32_t task_x, task_y;
    while (getTask(&task_x, &task_y)) 
    {
        placeSource(terrain, &list[task_y * COLUMNS_TERRAIN + task_x], task_x, task_y);
    }

    return 0;
}

int main(int argc, char *argv[]) 
{
    // Init data
    terrain = initializeTerrain();
    list = malloc(sizeof(Node*) * ROWS_TERRAIN * COLUMNS_TERRAIN);

    // Show terrain map
    //printTerrain(terrain);

    // Init threads
    LARGE_INTEGER start, end, freq;
    HANDLE threads[N_THREADS];

    InitializeCriticalSection(&cs);
    QueryPerformanceFrequency(&freq);
    
    // Compute
    last_task_x = 0; last_task_y = 0;
    QueryPerformanceCounter(&start);
    for(uint8_t i = 0; i < N_THREADS; i++) 
    {
        threads[i] = CreateThread(NULL, 0, threadFun, NULL, 0, NULL);
    }
    WaitForMultipleObjects(N_THREADS, threads, TRUE, INFINITE);
    QueryPerformanceCounter(&end);
    DeleteCriticalSection(&cs);

    double elapsedTime = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000.0;
    printf("execution time: %.3f ms\n", elapsedTime);

    for(uint8_t i = 0; i < N_THREADS; i++) {
        CloseHandle(threads[i]);
    }
    
    // Print all terrain maps
    /*printf("\n");
    for(uint16_t y = 0; y < ROWS_TERRAIN; y++) 
    {
        for(uint16_t x = 0; x < COLUMNS_TERRAIN; x++) 
        {
            printFloodedTerrain(terrain, list, x, y);
            printf("\n");
        }
    }*/
    
    // END
    freeList(list[4]);
    uninitializeTerrain(terrain);
}

void placeSource(uint8_t* terrain, Node** cellList, uint16_t x, uint16_t y)
{
    // Add node
    *cellList = makeNode(x, y);

    // Retrive cell value
    uint8_t cellValue = terrain[y * COLUMNS_TERRAIN + x];

    // Check neighbours
    for (int8_t i = -1, nextIndex = 0; i <= 1; i++) 
    {
        int8_t neighbourX = x + i;
        if (neighbourX < 0 || neighbourX >= COLUMNS_TERRAIN)
            continue;

        for (int8_t j = -1; j <= 1; j++) 
        {
            if (i == 0 && j == 0)
                // if i == 0 and j == 0, we are pointing to the current cell. Skip.
                continue;

            int8_t neighbourY = y + j;
            if (neighbourY < 0 || neighbourY >= ROWS_TERRAIN)
                continue;

            if (terrain[(neighbourY) * COLUMNS_TERRAIN + (neighbourX)] < cellValue)
            {
                // This cell has lower altidute, water propagates
                placeSource(terrain, &(*cellList)->next[nextIndex++], neighbourX, neighbourY);
            }
        }
    }
}

uint8_t random_uint8(uint8_t min, uint8_t max) {
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

uint8_t* initializeTerrain() 
{
    if (ROWS_TERRAIN == 0 || COLUMNS_TERRAIN == 0) {
        return 0;
    }

    uint8_t* terrain = malloc(sizeof(uint8_t*) * ROWS_TERRAIN * COLUMNS_TERRAIN);

    for(uint16_t i = 0; i < ROWS_TERRAIN; i++) {
        for(uint16_t j = 0; j < COLUMNS_TERRAIN; j++) {
            terrain[i * COLUMNS_TERRAIN + j] = random_uint8(MIN_ALTITUDE, MAX_ALTITUDE);
        }
    }
    return terrain;
}

void uninitializeTerrain(uint8_t* terrain) {
    free(terrain);
}

void printTerrain(uint8_t* terrain) 
{
    for(uint16_t i = 0; i < ROWS_TERRAIN; i++) {
        for(uint16_t j = 0; j < COLUMNS_TERRAIN; j++) {
            printf("%03d ", terrain[i * COLUMNS_TERRAIN + j]);
        }
        printf("\n");
    }
}

void printFloodedTerrain(uint8_t* terrain, Node** list, uint16_t x, uint16_t y) 
{
    Node* floodedCellList = list[y * COLUMNS_TERRAIN + x];

    for(uint16_t y1 = 0; y1 < ROWS_TERRAIN; y1++) 
    {
        for(uint16_t x1 = 0; x1 < COLUMNS_TERRAIN; x1++) 
        {
            if (x1 == x && y1 == y) {
                printf("\033[41;37m%03d \033[0m", terrain[y1 * COLUMNS_TERRAIN + x1]);
            }
            else if (findNode(floodedCellList, x1, y1)) {
                printf("\033[44;37m%03d \033[0m", terrain[y1 * COLUMNS_TERRAIN + x1]);
            }
            else {
                printf("\033[42;30m%03d \033[0m", terrain[y1 * COLUMNS_TERRAIN + x1]);
            }
        }
        printf("\n");
    }
}
