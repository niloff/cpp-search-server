#pragma once
#include "search_server.h"
#include "document.h"
#include <deque>
/**
 * Очередь запросов к серверу
 */
class RequestQueue {
public:
    /**
     * Конструктор
     */
    explicit RequestQueue(const SearchServer& search_server) :
        server_(search_server) { }
    /**
     * Добавить поисковой запрос
     */
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        const auto response = server_.FindTopDocuments(raw_query, document_predicate);
        int err_count = GetNoResultRequests();
        if(requests_.size() >= MINUTES_IN_DAY) {
            requests_.push_back({response, --err_count});
            requests_.pop_front();
            return response;
        }
        requests_.push_back({response, err_count});
        return response;
    }
    /**
     * Добавить поисковой запрос
     */
    std::vector<Document> AddFindRequest(const std::string& raw_query,
                                         DocumentStatus input_status = DocumentStatus::ACTUAL);
    /**
     * Количество неуспешных запросов к серверу
     */
    int GetNoResultRequests() const;
private:
    /**
     * Результат выполнения запроса
     */
    struct QueryResult {
        /**
         * Конструктор
         */
        QueryResult(const std::vector<Document> docs_result, int errors) :
            total_errors_(errors) {
            if(docs_result.empty()) {
                ++total_errors_;
            }
        }

        int total_errors_ = 0;
    };
    /**
     * Результаты запросов
     */
    std::deque<QueryResult> requests_;
    /**
     * Количество запросов в сутки
     */
    const static int MINUTES_IN_DAY = 1440;
    /**
     * Поисковой сервер
     */
    const SearchServer& server_;
};
