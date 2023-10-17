#include "process_queries.h"
#include <execution>
/**
 * Функция, распараллеливающая обработку
 * нескольких запросов к поисковой системе.
 */
std::vector<std::vector<Document>> ProcessQueries(
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    std::transform(std::execution::par,
                   queries.begin(), queries.end(),
                   result.begin(),
                   [&search_server](const std::string& query) {
        return search_server.FindTopDocuments(query);
    });
    return result;
}
/**
 * Функция, распараллеливающая обработку
 * нескольких запросов к поисковой системе.
 * Возвращает результат в "плоском" виде.
 */
std::list<Document> ProcessQueriesJoined(
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {
    std::list<Document> result;
    for(const auto& documents : ProcessQueries(search_server, queries)) {
        result.insert(result.end(), documents.begin(), documents.end());
    }
    return result;
}