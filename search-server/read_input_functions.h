#pragma once
#include <string>
/**
 * Функции для чтения ввода из консоли
 */
class ReadInput {
public:
    /**
     * Получить строку введённую через консоль
     */
    std::string ReadLine();
    /**
     * Получить число введённое через консоль
     */
    int ReadLineWithNumber();
};
