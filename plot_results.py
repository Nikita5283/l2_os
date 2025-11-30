import matplotlib.pyplot as plt

# Данные для процессов
process_counts = [1, 2, 4, 8, 16]
process_times = [3.132375, 1.667552, 1.651307, 1.149254, 1.122953]  # Средние времена для процессов

# Данные для потоков
thread_counts = [1, 2, 4, 8, 16]
thread_times = [1.391617, 1.332085, 1.312176, 1.459237, 1.423496]  # Средние времена для потоков

# Построение графика
plt.figure(figsize=(10, 6))
plt.plot(process_counts, process_times, marker='o', label='Процессы', color='blue')
plt.plot(thread_counts, thread_times, marker='o', label='Потоки', color='green')

# Настройка графика
plt.title('Сравнение времени выполнения: процессы vs потоки')
plt.xlabel('Количество процессов/потоков')
plt.ylabel('Среднее время выполнения (с)')
plt.xticks(process_counts)
plt.grid(True, linestyle='--', alpha=0.7)
plt.legend()

# Сохранение графика в файл и отображение
plt.savefig('comparison_plot.png', dpi=300)
plt.show()
