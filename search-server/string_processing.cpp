#include "string_processing.h"
#include <algorithm>

using namespace std;
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
bool StringProcessing::IsValidWord(const string_view word) {
    return none_of(word.begin(), word.end(), IsNonValidChar);
}
/**
 * Разложить входной текст в вектор из слов
 */
vector<string> StringProcessing::SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if(IsNonValidChar(c)) {
            throw invalid_argument(ERROR_INCORRECT_CHAR_WITH_CODE + " = '"s + to_string(c) + "'"s);
        }
        if(c != ' ') {
            word += c;
            continue;
        }
        if (word.empty()) {
            continue;
        }
        words.push_back(word);
        word.clear();
    }
    // добавляем последнее слово если оно есть
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}
/**
 * Разложить входной текст в вектор из слов
 */
std::vector<std::string_view> StringProcessing::SplitIntoWordsView(std::string_view text) {
    std::vector<std::string_view> words;
    text.remove_prefix(std::min(text.find_first_not_of(' '), text.size()));
    const std::string_view::size_type pos_end = std::string_view::npos;
    while (!text.empty()) {
        std::string_view::size_type space = text.find(' ');
        words.push_back(space == pos_end ? text.substr(0, text.size()) : text.substr(0, space));
        text.remove_prefix(std::min(text.find_first_not_of(' ', space), text.size()));
    }
    return words;
}
