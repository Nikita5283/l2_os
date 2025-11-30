#define _POSIX_C_SOURCE 199309L // Включаем POSIX 1993 для clock_gettime

#include <stdio.h> // printf, perror
#include <stdlib.h> // malloc, free, atoi, rand, srand, exit
#include <time.h> // clock_gettime
#include <sys/mman.h> // mmap, munmap, MAP_SHARED, MAP_ANONYMOUS
#include <sys/wait.h> // wait
#include <unistd.h> // fork, _exit
#include <string.h> // strcmp

typedef double val_t; // тип элементов в матрицах

// вспомогательная функция — таймер в секундах
static inline double now_sec() {
    struct timespec t; // структура для времени
    clock_gettime(CLOCK_REALTIME, &t); // читаем время
    return t.tv_sec + t.tv_nsec * 1e-9; // возвращаем в секундах
}

int main(int argc, char **argv) {
    int n = 400;                // размер матриц по умолчанию (можно увеличить)
    int procs = 4;              // число процессов по умолчанию
    int repeats = 3;            // число повторов для усреднения

    // парсим аргументы командной строки
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-n") && i+1<argc) n = atoi(argv[++i]);     // -n размер
        else if (!strcmp(argv[i], "-p") && i+1<argc) procs = atoi(argv[++i]); // -p процессы
        else if (!strcmp(argv[i], "-r") && i+1<argc) repeats = atoi(argv[++i]); // -r повторы
        else if (!strcmp(argv[i], "-h")) { 
            printf("Использование: %s [-n размер] [-p процессы] [-r повторы]\n", argv[0]); 
            return 0; 
        }
    }
    if (procs < 1) procs = 1;    // защита от неправильного ввода

    size_t total = (size_t)n * n; // общее число элементов

    // выделяем разделяемую память для A, B, C — чтобы дети и родитель видели одни и те же страницы
    val_t *A = mmap(NULL, sizeof(val_t)*total, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    val_t *B = mmap(NULL, sizeof(val_t)*total, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    val_t *C = mmap(NULL, sizeof(val_t)*total, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (A == MAP_FAILED || B == MAP_FAILED || C == MAP_FAILED) { perror("mmap"); return 1; } // проверка mmap

    // Заполняем A и B случайными значениями в разделяемой памяти (не учитывается в замере)
    srand(12345);                // фиксируем зерно генератора
    for (size_t i = 0; i < total; ++i) {
        A[i] = (rand() % 100) / 10.0;  // значения 0.0..9.9
        B[i] = (rand() % 100) / 10.0;
        C[i] = 0.0;                     // заранее обнуляем C
    }

    double times_sum = 0.0;      // сумма времён для усреднения
    for (int rep = 0; rep < repeats; ++rep) {
        // зануляем C перед каждым повтором
        for (size_t i = 0; i < total; ++i) C[i] = 0.0;

        // разбиение строк между процессами
        int base = n / procs;    // целое количество строк на процесс
        int rem = n % procs;     // остаток строк
        int cur = 0;             // текущий индекс строки

        double t0 = now_sec();   // старт замера
        for (int pid = 0; pid < procs; ++pid) {
            int rows = base + (pid < rem); // распределяем остаток
            int start = cur;               // начало диапазона
            int end = cur + rows;          // конец диапазона (не включительно)
            cur += rows;                   // двигаем указатель на строки для следующего процесса

            pid_t p = fork();              // создаём дочерний процесс
            if (p == -1) { perror("fork"); return 1; } // обработка ошибки fork
            if (p == 0) {                  // код дочернего процесса
                // дочерний процесс вычисляет свои строки и пишет в разделяемую C
                for (int i = start; i < end; ++i) {
                    val_t *Ai = A + (size_t)i * n;   // указатель на строку A[i]
                    val_t *Ci = C + (size_t)i * n;   // указатель на строку C[i]
                    for (int j = 0; j < n; ++j) {
                        val_t sum = 0.0;              // аккумулируем сумму
                        for (int k = 0; k < n; ++k)
                            sum += Ai[k] * B[(size_t)k * n + j]; // A[i][k] * B[k][j]
                        Ci[j] = sum;                  // записываем C[i][j] в разделяемую память
                    }
                }
                _exit(0); // завершаем дочерний процесс корректно (без вызова atexit)
            }
            // родитель продолжает создавать следующие процессы
        }

        // родитель ждёт завершения всех дочерних процессов
        for (int i = 0; i < procs; ++i) wait(NULL);

        double t1 = now_sec();   // конец замера
        double dt = t1 - t0;     // время одного прогона
        printf("Повтор %d: %.6f с\n", rep+1, dt); // выводим время
        times_sum += dt;         // аккумулируем
    }

    printf("Среднее время: %.6f с (n=%d процессы=%d повторы=%d)\n", times_sum / repeats, n, procs, repeats); // среднее

    // освобождаем разделяемую память
    munmap(A, sizeof(val_t)*total);
    munmap(B, sizeof(val_t)*total);
    munmap(C, sizeof(val_t)*total);
    return 0; // успешное завершение
}
