#include <stdio.h>
#include <stdlib.h>
#include <time.h>


/// @brief Выводит сообщение об ошибке в стандартный вывод и в лог-файл.
/// @param message Сообщение об ошибке, которое нужно вывести.
void print_error(const char *message)
{
    fprintf(stdout, "%s\n", message);

    FILE *file = fopen(".\\log.txt", "a");
    if (file == NULL) 
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%s\n", message);
    fclose(file);
    
    exit(EXIT_FAILURE);
}



/// @brief Выводит сообщение d стандартный вывод и в лог-файл.
/// @param message Сообщение, которое нужно вывести.
void print_log(const char *message)
{
    fprintf(stdout, "%s\n", message);

    FILE *file = fopen("./log.txt", "a");
    if (file == NULL) 
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%s\n", message);
    fclose(file);
}



/// @brief Получение текущего времени
/// @return Текущее время
char *get_time()
{
    time_t current_time;
    struct tm *time_info;
    time(&current_time);  // Получаем количество секунд с 1970-01-01 (UNIX время)
    time_info = localtime(&current_time);  // Преобразуем в структуру для локального времени

    // Выделяем память для строки с датой
    char *time_string = (char *)malloc(100 * sizeof(char));  // Выделение памяти для строки

    // Проверяем успешность выделения памяти
    if (time_string == NULL) {
        perror("Ошибка выделения памяти");
        exit(EXIT_FAILURE);  // Завершаем программу с ошибкой
    }

    // Форматируем время в строку
    strftime(time_string, 100, "%Y-%m-%d %H:%M:%S", time_info);

    return time_string;
}
