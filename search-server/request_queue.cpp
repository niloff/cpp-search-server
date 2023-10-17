#include "request_queue.h"
/**
 * Добавить поисковой запрос
 */
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query,
                                                   DocumentStatus input_status) {
     return RequestQueue::AddFindRequest(raw_query,
                                         [input_status]
                                         (int, DocumentStatus status, int) {
         return status == input_status;
     });
}
/**
 * Количество неуспешных запросов к серверу
 */
int RequestQueue::GetNoResultRequests() const {
    if(requests_.empty()) return 0;
    return requests_.back().total_errors_;
}
/**
 * Конструктор
 */
RequestQueue::QueryResult::QueryResult(const std::vector<Document> docs_result, int errors) :
    total_errors_(errors) {
    if(docs_result.empty()) {
        ++total_errors_;
    }
}