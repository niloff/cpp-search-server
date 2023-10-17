#include "remove_duplicates.h"
#include <iostream>

using namespace std;
/**
 * Поиск и удаление дубликатов на сервере
 */
void RemoveDuplicates(SearchServer &search_server) {
    set<vector<string_view>> words_known;
    set<int> ids_duplicate;
    for (const int document_id : search_server) {
        const auto &words_doc = search_server.GetUniqueWords(document_id);
        if(words_known.count(words_doc) != 0) {
            ids_duplicate.insert(document_id);
            continue;
        }
        words_known.insert(words_doc);
    }
    for(const int document_id : ids_duplicate) {
        cout << "Found duplicate document id "s << document_id << endl;
        search_server.RemoveDocument(document_id);
    }
}
