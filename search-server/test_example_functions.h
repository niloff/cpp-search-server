#pragma once

#include "search_server.h"
/**
 * Добавить новый документ с id, содержимым, статусом и оценками рейтинга
 */
void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);
/**
 * Найти документы, отсортированные по релевантности запросу
 */
void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query);
/**
 * Поиск всех документов соответствующих запросу
 */
void MatchDocuments(const SearchServer& search_server, const std::string& query);
/**
 * Вывод параметров документа в консоль
 */
void PrintDocument(const Document& document);
/**
 * Вывод параметров документа и найденных по запросу слов в консоль
 */
void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status);
