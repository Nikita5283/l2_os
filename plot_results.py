import matplotlib.pyplot as plt

# Данные для процессов (примерные значения, замените на реальные данные из вывода программы)
process_counts = [1, 2, 4, 8, 16]
process_times = [12.34, 6.78, 3.45, 2.89, 3.12]  # Средние времена для процессов

# Данные для потоков (примерные значения, замените на реальные данные из вывода программы)
thread_counts = [1, 2, 4, 8, 16]
thread_times = [11.56, 5.67, 2.98, 2.45, 2.67]  # Средние времена для потоков

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
