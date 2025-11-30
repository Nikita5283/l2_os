#define _POSIX_C_SOURCE 199309L // Включаем POSIX 1993 года для корректной работы clock_gettime

#include <stdio.h> // printf, fprintf
#include <stdlib.h> // malloc, free, atoi, rand, srand
#include <time.h> // clock_gettime, struct timespec
#include <pthread.h> // pthread_create, pthread_join

typedef double val_t; // тип элементов матрицы (double для точности)

// структура аргументов, передаваемых в поток
typedef struct {
    int id; // идентификатор потока (необязательно, для отладки)
    int n; // размер матрицы n x n
    int start_row; // индекс первой строки (включительно) для этого потока
    int end_row; // индекс строки (исключительно) до которой работать
    val_t *A; // указатель на матрицу A (в виде flat array)
    val_t *B; // указатель на матрицу B (в виде flat array)
    val_t *C; // указатель на матрицу C (в виде flat array)
} thread_arg_t;

// worker-функция, которая запускается в каждом потоке
void *worker(void *arg) {
    thread_arg_t *t = (thread_arg_t*)arg; // приводим void* к нужной структуре
    int n = t->n; // читаем локальную копию размера
    val_t *A = t->A; // локальная ссылка на A
    val_t *B = t->B; // локальная ссылка на B
    val_t *C = t->C; // локальная ссылка на C

    // внешний цикл — по строкам, которые назначены этому потоку
    for (int i = t->start_row; i < t->end_row; ++i) {
        val_t *Ci = C + (size_t)i * n; // указатель на начало i-й строки в C
        val_t *Ai = A + (size_t)i * n; // указатель на начало i-й строки в A
        // внутренний цикл — по столбцам результата
        for (int j = 0; j < n; ++j) {
            val_t sum = 0.0; // аккумулируем сумму произведений
            // цикл по k для вычисления скалярного произведения строки A[i] и столбца B[:,j]
            for (int k = 0; k < n; ++k) {
                sum += Ai[k] * B[(size_t)k * n + j]; // A[i][k] * B[k][j]
            }
            Ci[j] = sum; // записываем computed C[i][j]
        }
    }
    return NULL; // поток завершает выполнение
}

// таймер в секундах
static inline double now_sec() {
    struct timespec t;                                  
    clock_gettime(CLOCK_REALTIME, &t); // читаем реальное время
    return t.tv_sec + t.tv_nsec * 1e-9; // возвращаем значение в секундах (double)
}

int main(int argc, char **argv) {
    int n = 800; // размер матрицы по умолчанию (можно менять)
    int threads = 4; // количество потоков по умолчанию
    int repeats = 3; // число повторов для усреднения

    // разбор аргументов командной строки
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-n") && i+1<argc) n = atoi(argv[++i]); // -n размер
        else if (!strcmp(argv[i], "-t") && i+1<argc) threads = atoi(argv[++i]); // -t потоки
        else if (!strcmp(argv[i], "-r") && i+1<argc) repeats = atoi(argv[++i]); // -r повторы
        else if (!strcmp(argv[i], "-h")) {                                   
            printf("Использование: %s [-n размер] [-t потоки] [-r повторы]\n", argv[0]);
            return 0;
        }
    }

    if (threads < 1) threads = 1; // защита от некорректного ввода

    size_t total = (size_t)n * n; // общее количество элементов в матрице
    val_t *A = malloc(sizeof(val_t) * total); // выделяем память под A (flat)
    val_t *B = malloc(sizeof(val_t) * total); // выделяем под B
    val_t *C = malloc(sizeof(val_t) * total); // выделяем под C
    if (!A || !B || !C) { perror("malloc"); return 1; } // проверка успешности malloc

    // генерация матриц
    srand(12345); // фиксируем зерно генератора для воспроизводимости
    for (size_t i = 0; i < total; ++i) {
        A[i] = (rand() % 100) / 10.0; // значения 0.0 .. 9.9
        B[i] = (rand() % 100) / 10.0;
        C[i] = 0.0; // обнуляем C заранее
    }

    pthread_t *tid = malloc(sizeof(pthread_t) * threads); // массив идентификаторов потоков
    thread_arg_t *targs = malloc(sizeof(thread_arg_t) * threads); // аргументы для каждого потока

    double times_sum = 0.0; // усреднённое время
    for (int rep = 0; rep < repeats; ++rep) {
        // зануляем C перед каждым повтором (чтобы не влияли предыдущие результаты)
        for (size_t i = 0; i < total; ++i) C[i] = 0.0;

        // разбиение строк между потоками: базовое число строк + равномерное распределение остатка
        int base = n / threads; // целая часть строк на поток
        int rem = n % threads; // остаток (нужно распределить между первыми rem потоками)
        int cur = 0; // текущая начальная строка для следующего потока
        for (int i = 0; i < threads; ++i) {
            int rows = base + (i < rem); // если i < rem — даём по 1 дополнительной строке
            targs[i].id = i; // сохраняем id (для отладки)
            targs[i].n = n; // сохраняем n
            targs[i].start_row = cur; // начало диапазона
            targs[i].end_row = cur + rows; // конец диапазона (не включительно)
            targs[i].A = A; // передаём указатели на данные
            targs[i].B = B;
            targs[i].C = C;
            cur += rows; // сдвигаем текущую строку
        }

        double t0 = now_sec(); // старт замера
        for (int i = 0; i < threads; ++i) pthread_create(&tid[i], NULL, worker, &targs[i]); // создаём потоки
        for (int i = 0; i < threads; ++i) pthread_join(tid[i], NULL); // ждём завершения
        double t1 = now_sec(); // конец замера
        double dt = t1 - t0; // время одного повтора
        printf("Повтор %d: %.6f с\n", rep+1, dt); // выводим время повтора
        times_sum += dt; // прибавляем ко времени для усреднения
    }

    printf("Среднее время: %.6f с (n=%d потоки=%d повторы=%d)\n", times_sum / repeats, n, threads, repeats); // вывод среднего

    free(A); free(B); free(C); free(tid); free(targs); // освобождаем всю выделенную память
    return 0; // успешное завершение программы
}
