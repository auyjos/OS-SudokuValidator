#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <omp.h>
#include <sys/syscall.h>

#define SIZE 9

char sudoku[SIZE][SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Declaración de funciones
void loadSudoku(char *filename);
void validateRows();
void validateColumns();
void validateSubgrids(int startRow, int startCol);
void *columnValidation(void *arg);
int isSudokuValid();

int main(int argc, char *argv[])
{

    // Establecer el anidamiento de OpenMP al inicio de la función
    omp_set_nested(1);
    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <nombre_del_archivo>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Establece el número de threads al inicio de main
    omp_set_num_threads(1);

    loadSudoku(argv[1]);

    // Obtener el número de threads en ejecución
    printf("El thread en el que se ejecuta main es: %ld\n", syscall(SYS_gettid));

    // Crear pthread para la validación de columnas
    pthread_t columnThread;
    if (pthread_create(&columnThread, NULL, columnValidation, NULL) != 0)
    {

        perror("Error creando el thread");
        exit(EXIT_FAILURE);
    }

    // Esperar a que termine el thread de validación de columnas
    if (pthread_join(columnThread, NULL) != 0)
    {
        perror("Error esperando al thread");
        exit(EXIT_FAILURE);
    }

    printf("Antes de terminar main(), el estado de este proceso y sus threads es:\n");
    system("ps -p $(ps -o ppid= -p $$) -L");

    // Realizar la validación de filas y subgrids
    validateRows();

    // Habilitar el anidamiento de threads
    omp_set_nested(1);

    // Desplegar mensaje de sudoku resuelto si es válido
    if (isSudokuValid())
    {
        printf("Sudoku resuelto!\n");
    }
    else
    {
        printf("La solución del sudoku no es válida.\n");
    }

    // Obtener información sobre el proceso y sus threads antes de terminar
    printf("Antes de terminar, el estado de este proceso y sus threads es:\n");
    system("ps -p $(ps -o ppid= -p $$) -L");

    return 0;
}

void loadSudoku(char *filename)
{
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        perror("Error abriendo el archivo");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    fstat(fd, &st);

    char *file_content = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_content == MAP_FAILED)
    {
        perror("Error mapeando el archivo");
        exit(EXIT_FAILURE);
    }

    int index = 0;
    for (int i = 0; i < SIZE; ++i)
    {
        for (int j = 0; j < SIZE; ++j)
        {
            sudoku[i][j] = file_content[index++] - '0';
        }
    }

    munmap(file_content, st.st_size);
    close(fd);
}

void validateRows()
{
    // Establecer el anidamiento de OpenMP al inicio de la función
    omp_set_nested(1);
    // Establece el número de threads para el for paralelo
    omp_set_num_threads(SIZE);

    // Variable para indicar si se ha encontrado un error
    int error = 0;

    // Agregar la directiva schedule(dynamic) a la línea #pragma omp parallel for
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < SIZE; ++i)
    {
        int seen[SIZE + 1] = {0}; // Marcar los números que ya se han visto en esta fila

        // Declarar i y j dentro del bucle
        for (int j = 0; j < SIZE; ++j)
        {
            int num = sudoku[i][j];

            if (seen[num])
            {
#pragma omp critical
                {
                    printf("Fila %d no es válida.\n", i + 1);
                    error = 1; // Establecer la variable de error
                }
            }

            seen[num] = 1;
        }
    }

    if (error)
    {
#pragma omp critical
        printf("Se encontraron errores en las filas.\n");
    }
    else
    {
#pragma omp critical
        printf("Todas las filas son válidas.\n");
    }
}

void validateColumns()
{

    // Establecer el anidamiento de OpenMP al inicio de la función
    omp_set_nested(1);
    // Establece el número de threads para el for paralelo
    omp_set_num_threads(SIZE);

    // Variable para indicar si se ha encontrado un error
    int error = 0;

    // Agregar la directiva schedule(dynamic) a la línea #pragma omp parallel for
#pragma omp parallel for schedule(dynamic)
    for (int j = 0; j < SIZE; ++j)
    {
        int seen[SIZE + 1] = {0}; // Marcar los números que ya se han visto en esta columna

        // Declarar i y j dentro del bucle
        for (int i = 0; i < SIZE; ++i)
        {
            int num = sudoku[i][j];

            if (seen[num])
            {
#pragma omp critical
                {
                    printf("Columna %d no es válida.\n", j + 1);
                    error = 1; // Establecer la variable de error
                }
            }

            seen[num] = 1;
        }
    }

    if (error)
    {
#pragma omp critical
        printf("Se encontraron errores en las columnas.\n");
    }
    else
    {
#pragma omp critical

        printf("Todas las columnas son válidas.\n");
    }
}

void validateSubgrids(int startRow, int startCol)
{
    // Establecer el anidamiento de OpenMP al inicio de la función
    omp_set_nested(1);

    // Variable para indicar si se ha encontrado un error
    int error = 0;

    // Agregar la directiva schedule(dynamic) a la línea #pragma omp parallel for
#pragma omp parallel for schedule(dynamic)
    for (int i = startRow; i < startRow + 3; ++i)
    {
        for (int j = startCol; j < startCol + 3; ++j)
        {
            int seen[SIZE + 1] = {0}; // Marcar los números que ya se han visto en este subarreglo

            // Declarar i y j dentro del bucle
            for (int row = 0; row < 3; ++row)
            {
                for (int col = 0; col < 3; ++col)
                {
                    int num = sudoku[i + row][j + col];

                    if (seen[num])
                    {
#pragma omp critical
                        {
                            printf("Subarreglo [%d, %d] no es válido.\n", i, j);
                            error = 1; // Establecer la variable de error
                        }
                    }

                    seen[num] = 1;
                }
            }
        }
    }

    if (error)
    {
#pragma omp critical
        printf("Se encontraron errores en los subarreglos.\n");
    }
    else
    {
#pragma omp critical
        printf("Todos los subarreglos son válidos.\n");
    }
}

void *columnValidation(void *arg)
{

    // Establecer el anidamiento de OpenMP al inicio de la función
    omp_set_nested(1);
    // Establecer el número de threads para el for paralelo
    omp_set_num_threads(SIZE);

    // Variable para indicar si se ha encontrado un error
    int error = 0;

    // Agregar la directiva schedule(dynamic) a la línea #pragma omp parallel for
#pragma omp parallel for schedule(dynamic)
    for (int j = 0; j < SIZE; ++j)
    {
        int seen[SIZE + 1] = {0}; // Marcar los números que ya se han visto en esta columna

        // Declarar i y j dentro del bucle
        for (int i = 0; i < SIZE; ++i)
        {
            int num = sudoku[i][j];

            if (seen[num])
            {
#pragma omp critical
                {
                    printf("Columna %d no es válida (desde el thread).\n", j + 1);
                    error = 1; // Establecer la variable de error
                }
            }

            seen[num] = 1;
        }
    }

    if (error)
    {
#pragma omp critical
        printf("Se encontraron errores en las columnas (desde el thread).\n");
    }
    else
    {
#pragma omp critical
        printf("Todas las columnas son válidas (desde el thread).\n");
    }

    return NULL;
}

int isSudokuValid()
{
    // Realizar la verificación de la validez global del sudoku

    for (int i = 0; i < SIZE; ++i)
    {
        for (int j = 0; j < SIZE; ++j)
        {
            // Verificar subgrids
            if (i % 3 == 0 && j % 3 == 0)
            {
                validateSubgrids(i, j);
            }
        }
    }

    // Verificar filas y columnas
    validateRows();
    validateColumns();

    // Agregar lógica adicional según tus necesidades

    return 1; // Cambiar según la implementación real
}
