#pragma once
#include <string>
#include <vector>
#include <set>
#include <stdexcept>
/**
 * Класс для строковых преобразований
 */
class StringProcessing {
public:
    /**
     * Является ли символ невалидным
     */
    static bool IsNonValidChar(const char ch);
    /**
     * Проверить символы слова на валидность
     */
    static bool IsValidWord(const std::string& word);
    /**
     * Разложить входной текст в вектор из слов
     */
    static std::vector<std::string> SplitIntoWords(const std::string& text);
    /**
     * Преобразовать контейнер в набор из непустых слов
     */
    template <typename Container>
    static std::set<std::string> ToNonEmptySet(const Container& container);
private:
    /**
     * Описание ошибки - недопустимый код символа
     */
    static const char* ERROR_INCORRECT_CHAR_WITH_CODE;
    /**
     * Описание ошибки - недопустимый код символа в слове
     */
    static const char* ERROR_INCORRECT_WORD;
};
/**
 * Преобразовать контейнер в набор из непустых слов
 */
template <typename Container>
std::set<std::string> StringProcessing::ToNonEmptySet(const Container& container) {
    using namespace std::literals;
    std::set<std::string> result;
    for (const std::string& word : container) {
        if(!IsValidWord(word)) {
            throw std::invalid_argument(ERROR_INCORRECT_WORD + " = '"s + word + "'"s);
        }
        if (word.empty()) {
            continue;
        }
        result.insert(word);
    }
    return result;
}
