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
    static bool IsValidWord(const std::string_view word);
    /**
     * Разложить входной текст в вектор из слов
     */
    static std::vector<std::string> SplitIntoWords(const std::string& text);
    /**
     * Разложить входной текст в вектор из слов
     */
    static std::vector<std::string_view> SplitIntoWordsView(std::string_view str);
    /**
     * Преобразовать контейнер в набор из непустых слов
     */
    template <typename Container>
    static std::set<std::string, std::less<>> ToNonEmptySet(const Container& container);
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
std::set<std::string, std::less<> > StringProcessing::ToNonEmptySet(const Container& container) {
    using namespace std::literals;
    std::set<std::string, std::less<>> result;
    for (const auto word : container) {
        if(!IsValidWord(word)) {
            throw std::invalid_argument(std::string(ERROR_INCORRECT_WORD) + " = '"s + std::string(word) + "'"s);
        }
        if (!word.empty()) {
            result.emplace(word);
        }
    }
    return result;
}
