#include "document.h"
#include <math.h>
#include <numeric>
/**
 * Допустимая погрешность вычислений при округлении
 */
const double Document::ERROR_VALUE_EPSILON = 1e-6;
/**
 * Описание ошибки - некорректный идентификатор документа
 */
const char* Document::ERROR_DOCUMENT_ID = "Некорректный идентификатор документа";
/**
 * Описание ошибки - некорректный идентификатор документа
 */
const char* Document::ERROR_DOCUMENT_INDEX = "Некорректный индекс документа";
/**
 * Оператор сравнения <
 */
bool Document::operator<(const Document& doc) const {
    if (std::abs(relevance - doc.relevance) < ERROR_VALUE_EPSILON) {
        return rating > doc.rating;
    }
    return relevance > doc.relevance;
}
/**
 * Рассчитать средний рейтинг
 */
int DocumentData::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}
