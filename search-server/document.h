#pragma once
#include <vector>
#include <ostream>
/**
 * Документ
 */
class Document {
public:
    /**
     * Описание ошибки - некорректный идентификатор документа
     */
    static const char* ERROR_DOCUMENT_ID;
    /**
     * Описание ошибки - некорректный идентификатор документа
     */
    static const char* ERROR_DOCUMENT_INDEX;
public:
    Document() = default;
    Document(int id, double relevance, int rating):
        id(id),
        relevance(relevance),
        rating(rating) { }
    /**
     * Оператор сравнения <
     */
    bool operator<(const Document& doc) const;
    /**
     * Информация о документе в потоке вывода
     */
    friend std::ostream& operator<<(std::ostream& out, const Document& document) {
        using namespace std;
        out << "{ "s
            << "document_id = "s << document.id << ", "s
            << "relevance = "s << document.relevance << ", "s
            << "rating = "s << document.rating << " }"s;
        return out;
    }
private:
    /**
     * Идентификатор
     */
    int id = 0;
    /**
     * Релевантность
     */
    double relevance = 0.0;
    /**
     * Рейтинг
     */
    int rating = 0;
};
/**
 * Cтатус документа
 */
enum class DocumentStatus {
    /**
     * Актуальный
     */
    ACTUAL,
    /**
     * Не соотвествующий запросу
     */
    IRRELEVANT,
    /**
     * Исключен
     */
    BANNED,
    /**
     * Удален
     */
    REMOVED,
};
/**
 * Данные документа
 */
struct DocumentData {
    DocumentData() = default;
    DocumentData(std::vector<int> ratings, DocumentStatus s) :
        rating(ComputeAverageRating(ratings)),
        status(s) {}
    /**
     * Рассчитать средний рейтинг
     */
    static int ComputeAverageRating(const std::vector<int>& ratings);
    /**
     * Рейтинг
     */
    int rating;
    /**
     * Статус
     */
    DocumentStatus status;
};
