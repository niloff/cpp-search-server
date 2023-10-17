#pragma once
#include "concurrent_map.h"
#include "string_processing.h"
#include "document.h"
#include <string>
#include <set>
#include <map>
#include <tuple>
#include <thread>
#include <algorithm>
#include <execution>
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
private:
    /**
     * Пустой контейнер слов и text frequency
     */
    static const std::map<std::string_view, double> EMPTY_DOC_MEASURES;
public:
    /**
     * Конструктор
     */
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    /**
     * Конструктор
     */
    explicit SearchServer(const std::string& stop_words_text):
        SearchServer(StringProcessing::SplitIntoWordsView(stop_words_text)) { }
    /**
     * Конструктор
     */
    explicit SearchServer(std::string_view stop_words_text):
        SearchServer(StringProcessing::SplitIntoWordsView(stop_words_text)) { }
    /**
     * Начальный итератор загруженных id документов
     */
    const std::set<int>::const_iterator begin() const noexcept;
    /**
     * Конечный итератор загруженных id документов
     */
    const std::set<int>::const_iterator end() const noexcept;
    /**
     * Добавить новый документ с id, содержимым, статусом и оценками рейтинга
     */
    void AddDocument(int document_id,
                     std::string_view document,
                     DocumentStatus status,
                     const std::vector<int>& ratings);
    /**
     * Найти документы, отсортированные по релевантности запросу
     * Вариант с политикой исполения поиска (однопоточная/многопоточная) и
     * функциональным объектом в качестве параметра
     * Выводит максимум MAX_RESULT_DOCUMENT_COUNT документов
     */
    template<typename ExecutionPolicy, typename Functor>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy,
                                           std::string_view raw_query,
                                           Functor functor) const;
    /**
     * Найти документы, отсортированные по релевантности запросу
     * Вариант с функциональным объектом в качестве параметра
     * Выводит максимум MAX_RESULT_DOCUMENT_COUNT документов
     */
    template <typename Functor>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, Functor functor) const;
    /**
     * Найти документы, отсортированные по релевантности запросу
     * Вариант с политикой исполения поиска в качестве параметра и
     * статуса документа
     * Выводит максимум MAX_RESULT_DOCUMENT_COUNT документов
     */
    template<typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy,
                                           std::string_view raw_query,
                                           DocumentStatus input_status = DocumentStatus::ACTUAL) const;
    /**
    * Найти документы, отсортированные по релевантности запросу
    * Вариант со статусом документа в качестве параметра
    * Выводит максимум MAX_RESULT_DOCUMENT_COUNT документов
    */
    std::vector<Document> FindTopDocuments(std::string_view raw_query,
                                           DocumentStatus input_status = DocumentStatus::ACTUAL) const;
    /**
     * Количество загруженных документов
     */
    int GetDocumentCount() const;
    /**
     * Совпадающие слова в запросе к конкретному документу и статус документа.
     */
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query,
                                                                       int document_id) const;
    /**
     * Совпадающие слова в запросе к конкретному документу и статус документа.
     * Последовательная реализация.
     */
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&,
                                                                       std::string_view raw_query,
                                                                       int document_id) const;
    /**
     * Совпадающие слова в запросе к конкретному документу и статус документа.
     * Многопоточная реализация
     */
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&,
                                                                       std::string_view raw_query,
                                                                       int document_id) const;
    /**
     * Получить text frequency слов по id документа
     */
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    /**
     * Получить уникальные слова документа
     */
    const std::vector<std::string_view> GetUniqueWords(int document_id) const;
    /**
     * Удалить документ по его id
     */
    void RemoveDocument(int document_id);
    /**
     * Удалить документ по его id
     * Последовательная реализация
     */
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    /**
     * Удалить документ по его id
     * Многопоточная реализация
     */
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);
private:
    /**
     * Слово из запроса
     */
    struct QueryWord {
        /**
         * Содержимое слова
         */
        std::string_view text;
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
        std::vector<std::string_view> words_plus;
        /**
         * Минус-слова
         */
        std::vector<std::string_view> words_minus;
    };
    /**
     * Измерения для слов загруженных документов
     * Содержит id документов, где они встречаются и text frequency
     */
    std::map<std::string, std::map<int, double>> words_measures_;
    /**
     * Измерения для загруженных документов
     * По id документа содержит слова и их text frequency
     */
    std::map<int, std::map<std::string_view, double>> document_measures_;
    /**
     * Известные стоп-слова
     */
    const std::set<std::string, std::less<>> stop_words_;
    /**
     * Количество загруженных документов
     */
    std::map<int, DocumentData> documents_;
    /**
     * Идентификаторы добавленных документов
     */
    std::set<int> document_ids_;
    /**
     * Является ли слово стоп-словом
     */
    bool IsStopWord(std::string_view word) const;
    /**
     * Разложить входной текст в вектор из слов, исключая известные стоп-слова
     */
    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;
    /**
     * Получить слово запроса из текста
     */
    QueryWord ParseQueryWord(std::string_view text) const;
    /**
     * Получить структурированный запрос из текста
     */
    Query ParseQuery(std::string_view text) const;
    /**
     * Получить структурированный запрос из текста.
     * Реализация под многопоточность
     */
    Query ParseQueryParallel(std::string_view text) const;
    /**
     * Вычислить IDF для слова
     */
    double CalcIdf(std::string_view word) const;
    /**
     * Содержатся ли минус-слова в документе
     */
    bool IsDocHasMinus(const std::pair<int, double>& doc,
                       const std::vector<std::string_view> &words_minus) const;
    /**
     * Найти все документы, соответствующие запросу
     * Для документов также расчитывается TF-IDF
     */
    template<typename Functor>
    std::vector<Document> FindAllDocuments(const Query& query, Functor functor) const;
    /**
     * Найти все документы, соответствующие запросу
     * Многопоточная реализация
     * Для документов также расчитывается TF-IDF
     */
    template<typename ExecutionPolicy, typename Functor>
    std::vector<Document> FindAllDocuments(ExecutionPolicy policy, const Query& query, Functor functor) const;
};
/**
 * Конструктор
 */
template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words):
    stop_words_(StringProcessing::ToNonEmptySet(stop_words)) { }
/**
 * Найти документы, отсортированные по релевантности запросу
 * Вариант с политикой исполения поиска (однопоточная/многопоточная) и
 * функциональным объектом в качестве параметра
 * Выводит максимум MAX_RESULT_DOCUMENT_COUNT документов
 */
template<typename ExecutionPolicy, typename Functor>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query, Functor functor) const {
    const Query& query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, functor);
    sort(matched_documents.begin(), matched_documents.end());
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}
/**
 * Найти документы, отсортированные по релевантности запросу
 * Вариант с функциональным объектом в качестве параметра
 * Выводит максимум MAX_RESULT_DOCUMENT_COUNT документов
 */
template <typename Functor>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, Functor functor) const {
    return FindTopDocuments(std::execution::seq, raw_query, functor);
}
/**
 * Найти документы, отсортированные по релевантности запросу
 * Вариант с политикой исполения поиска в качестве параметра и
 * статуса документа
 * Выводит максимум MAX_RESULT_DOCUMENT_COUNT документов
 */
template<typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy,
                                       std::string_view raw_query,
                                       DocumentStatus input_status) const {
    return FindTopDocuments(policy,
                            raw_query,
                            [input_status](int, DocumentStatus status, int) {
        return status == input_status;
    });
}
/**
 * Найти все документы, соответствующие запросу
 * Для документов также расчитывается TF-IDF
 */
template<typename Functor>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, Functor functor) const {
    std::map<int, double> relevances;
    for (const auto word_plus : query.words_plus) {
        const auto word_it = words_measures_.find(std::string(word_plus));
        if(word_it == words_measures_.cend()) continue;
        const double idf = CalcIdf(word_plus);
        for(const auto [doc_id, tf] : word_it->second) {
            const DocumentData &doc_data = documents_.at(doc_id);
            if(!functor(doc_id, doc_data.status, doc_data.rating)) {
                continue;
            }
            relevances[doc_id] += tf * idf;
        }
    }
    std::vector<Document> matched_documents;
    for (const auto& [doc_id, relevance] : relevances) {
        if(IsDocHasMinus({doc_id, relevance}, query.words_minus)) {
            continue;
        }
        matched_documents.push_back({doc_id, relevance, documents_.at(doc_id).rating});
    }
    return matched_documents;
}
/**
 * Найти все документы, соответствующие запросу
 * Многопоточная реализация
 * Для документов также расчитывается TF-IDF
 */
template<typename ExecutionPolicy, typename Functor>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy policy, const Query& query, Functor functor) const {
    using namespace std::execution;
//    // определяем доступное число потоков
//    const auto thread_count = static_cast<int>(std::thread::hardware_concurrency());
    ConcurrentMap<int, double> relevances(16);
    // Многопоточный расчёт релевантности документов
    std::for_each(policy,
                  query.words_plus.begin(), query.words_plus.end(),
                  [this, &functor, &relevances](std::string_view word) {
        const auto word_it = words_measures_.find(std::string(word));
        if(word_it == words_measures_.cend()) return;
        const double idf = CalcIdf(word);
        for (const auto [doc_id, tf] : word_it->second) {
            const auto& [rating, status] = documents_.at(doc_id);
            if (!functor(doc_id, status, rating)) continue;
            relevances[doc_id].ref_to_value += tf * idf;
        }
    });
    // Многопоточное удаление если документ содержит минус-слово
    std::for_each(policy,
                  query.words_minus.begin(), query.words_minus.end(),
                  [this, &functor, &relevances](std::string_view word) {
        const auto word_pos = words_measures_.find(std::string(word));
        if(word_pos == words_measures_.cend()) return;
        for (const auto [doc_id, _] : word_pos->second) {
            relevances.Erase(doc_id);
        }
    });
    // Преобразуем ConcurrentMap<k,v> в std::map<k,v>
    auto document_to_relevance = relevances.BuildOrdinaryMap();
    std::vector<Document> matched_documents;
    matched_documents.reserve(document_to_relevance.size());
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}
