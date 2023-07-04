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
bool StringProcessing::IsValidWord(const string& word) {
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
