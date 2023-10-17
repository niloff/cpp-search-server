#pragma once
#include "search_server.h"
#include <list>
/**
 * Функция, распараллеливающая обработку
 * нескольких запросов к поисковой системе.
 */
std::vector<std::vector<Document>> ProcessQueries(
        const SearchServer& search_server,
        const std::vector<std::string>& queries);
/**
 * Функция, распараллеливающая обработку
 * нескольких запросов к поисковой системе.
 * Возвращает результат в "плоском" виде.
 */
std::list<Document> ProcessQueriesJoined(
        const SearchServer& search_server,
        const std::vector<std::string>& queries);