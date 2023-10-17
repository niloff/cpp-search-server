#include "test_example_functions.h"
#include <iostream>

using namespace std;
/**
 * Добавить новый документ с id, содержимым, статусом и оценками рейтинга
 */
void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const exception& e) {
        cout << "Ошибка при добавлении документа ID = '"s << document_id << "': "s << e.what() << endl;
    }
}
/**
 * Найти документы, отсортированные по релевантности запросу
 */
void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    std::cout << "Результаты поиска по запросу '"s << raw_query << "'" << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const std::exception& e) {
        std::cout << "Ошибка поиска: "s << e.what() << std::endl;
    }
}
/**
 * Поиск всех документов соответствующих запросу
 */
void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    try {
        std::cout << "Поиск всех документов, соответствующих запросу '"s << query << "'" << std::endl;
        for (const int document_id : search_server) {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const std::exception& e) {
        std::cout << "Ошибка поиска документов: "s << e.what() << std::endl;
    }
}
/**
 * Вывод параметров документа в консоль
 */
void PrintDocument(const Document& document) {
    std::cout << document << std::endl;
}
/**
 * Вывод параметров документа и найденных по запросу слов в консоль
 */
void PrintMatchDocumentResult(int document_id, const std::vector<string_view> &words, DocumentStatus status) {
    std::cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast<int>(status) << ", "s
        << "words ="s;
    for (const auto& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}
