#pragma once
#include "string_processing.h"
#include "document.h"
#include <string>
#include <set>
#include <map>
#include <tuple>
#include <algorithm>
/**
 * Поисковой сервер
 */
class SearchServer {
public:
    /**
     * Значение идентификатора если документ отсуствует
     */
    inline static constexpr int INVALID_DOCUMENT_ID = -1;
    /**
     * Максимальное число документов в выдаче
     */
    static const int MAX_RESULT_DOCUMENT_COUNT = 5;
    /**
     * Описание ошибки - пустое минус-слово
     */
    static const char* ERROR_MINUS_WORD_EMPTY;
    /**
     * Описание ошибки - минус-слово содержит лишнее тире
     */
    static const char* ERROR_MINUS_WORD_EXTRADASH;
    /**
     * Конструктор
     */
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words):
        stop_words_(StringProcessing::ToNonEmptySet(stop_words)) { }
    /**
     * Конструктор
     */
    explicit SearchServer(const std::string& stop_words_text):
        SearchServer(StringProcessing::SplitIntoWords(stop_words_text)) { }
    /**
     * Добавить новый документ с id, содержимым, статусом и оценками рейтинга
     */
    void AddDocument(int document_id,
                     const std::string& document,
                     DocumentStatus status,
                     const std::vector<int>& ratings);
    /**
     * Найти документы, отсортированные по релевантности запросу
     * Вариант с функциональным объектом в качестве параметра
     * Выводит максимум MAX_RESULT_DOCUMENT_COUNT документов
     */
    template<typename Functor>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, Functor functor) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, functor);
        sort(matched_documents.begin(), matched_documents.end());
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    /**
    * Найти документы, отсортированные по релевантности запросу
    * Вариант со статусом документа в качестве параметра
    * Выводит максимум MAX_RESULT_DOCUMENT_COUNT документов
    */
    std::vector<Document> FindTopDocuments(const std::string& raw_query,
                                           DocumentStatus input_status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [input_status](int, DocumentStatus status, int) { return status == input_status; });
    }
    /**
     * Количество загруженных документов
     */
    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }
    /**
     * Совпадающие слова в запросе к конкретному документу и статус документа
     */
    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query,
                                                                       int document_id) const;
    /**
     * Идентификатор документа по его порядковому индексу
     */
    int GetDocumentId(int index) const;
private:
    /**
     * Слово из запроса
     */
    struct QueryWord {
        /**
         * Содержимое слова
         */
        std::string text;
        /**
         * Является ли минус-словом
         */
        bool is_minus;
        /**
         * Является ли стоп-словом
         */
        bool is_stop;
    };
    /**
     * Структурированный запрос
     */
    struct Query {
        /**
         * Плюс-слова
         */
        std::set<std::string> words_plus;
        /**
         * Минус-слова
         */
        std::set<std::string> words_minus;
    };
    /**
     * Измерения для слов загруженных документов
     * Содержит id документов, где они встречаются и text frequency
     */
    std::map<std::string, std::map<int, double>> words_measures_;
    /**
     * Известные стоп-слова
     */
    std::set<std::string> stop_words_;
    /**
     * Количество загруженных документов
     */
    std::map<int, DocumentData> documents_;
    /**
     * Идентификаторы документов в порядке их добавления
     */
    std::vector<int> document_ids_;
    /**
     * Является ли слово стоп-словом
     */
    bool IsStopWord(const std::string& word) const {
        return stop_words_.count(word) > 0;
    }
    /**
     * Разложить входной текст в вектор из слов, исключая известные стоп-слова
     */
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
    /**
     * Получить слово запроса из текста
     */
    QueryWord ParseQueryWord(std::string text) const;
    /**
     * Получить структурированный запрос из текста
     */
    Query ParseQuery(const std::string& text) const;
    /**
     * Вычислить IDF для слова
     */
    double CalcIdf(const std::string& word) const;
    /**
     * Содержатся ли минус-слова в документе
     */
    bool IsDocHasMinus(const std::pair<int, double>& doc,
                       const std::set<std::string> &words_minus) const;
    /**
     * Найти все документы, соответствующие запросу
     * Для документов также расчитывается TF-IDF
     */
    template<typename Functor>
    std::vector<Document> FindAllDocuments(const Query& query, Functor functor) const {
        std::map<int, double> relevances;
        for (const std::string &word_plus : query.words_plus) {
            if(words_measures_.count(word_plus) == 0) continue;
            const double idf = CalcIdf(word_plus);
            for(const auto [doc_id, tf] : words_measures_.at(word_plus)) {
                const DocumentData &doc_data = documents_.at(doc_id);
                if(!functor(doc_id, doc_data.status, doc_data.rating)) continue;
                relevances[doc_id] += tf * idf;
            }
        }
        std::vector<Document> matched_documents;
        for (const auto& [doc_id, relevance] : relevances) {
            if(IsDocHasMinus({doc_id, relevance}, query.words_minus)) continue;
            matched_documents.push_back({doc_id, relevance, documents_.at(doc_id).rating});
        }
        return matched_documents;
    }
};
