#include "document.h"
#include <math.h>
#include <numeric>

using namespace std;
/**
 * Описание ошибки - некорректный идентификатор документа
 */
const char* Document::ERROR_DOCUMENT_ID = "Некорректный идентификатор документа";
/**
 * Описание ошибки - некорректный индекс документа
 */
const char* Document::ERROR_DOCUMENT_INDEX = "Некорректный индекс документа";
/**
 * Оператор сравнения <
 */
bool Document::operator<(const Document& doc) const {
    if (abs(relevance - doc.relevance) < numeric_limits<double>::epsilon()) {
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
