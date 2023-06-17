#include "string_processing.h"
#include <algorithm>

/**
 * Описание ошибки - недопустимый код символа
 */
const char* StringProcessing::ERROR_INCORRECT_CHAR_WITH_CODE = "Недопустимый код символа";
/**
 * Описание ошибки - недопустимый код символа в слове
 */
const char* StringProcessing::ERROR_INCORRECT_WORD = "Недопустимый код символа в слове";
/**
 * Является ли символ невалидным
 */
bool StringProcessing::IsNonValidChar(const char ch) {
    return ch >= '\0' && ch < ' ';
}
/**
 * Проверить символы слова на валидность
 */
bool StringProcessing::IsValidWord(const std::string& word) {
    return std::none_of(word.begin(), word.end(), IsNonValidChar);
}
/**
 * Разложить входной текст в вектор из слов
 */
std::vector<std::string> StringProcessing::SplitIntoWords(const std::string& text) {
    using namespace std::literals;
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if(IsNonValidChar(c))
            throw std::invalid_argument(ERROR_INCORRECT_CHAR_WITH_CODE + " = '"s + std::to_string(c) + "'"s);
        if(c != ' ') {
            word += c;
            continue;
        }
        if (word.empty()) continue;
        words.push_back(word);
        word.clear();
    }
    // добавляем последнее слово если оно есть
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}
