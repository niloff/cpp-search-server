#include "search_server.h"
#include <math.h>

using namespace std;
/**
 * Описание ошибки - пустое минус-слово
 */
const char* SearchServer::ERROR_MINUS_WORD_EMPTY = "В запросе содержится пустое минус-слово";
/**
 * Описание ошибки - минус-слово содержит лишнее тире
 */
const char* SearchServer::ERROR_MINUS_WORD_EXTRADASH = "Минус-слово содержит лишнее тире";
/**
 * Добавить новый документ с id, содержимым, статусом и оценками рейтинга
 */
void SearchServer::AddDocument(int document_id,
                 const std::string& document,
                 DocumentStatus status,
                 const std::vector<int>& ratings) {
    if(document_id < 0 || documents_.count(document_id))
        throw invalid_argument(Document::ERROR_DOCUMENT_ID + " = '"s + to_string(document_id) + "'"s);
    const vector<string> words = SplitIntoWordsNoStop(document);
    document_ids_.push_back(document_id); // добавляем id документа в список добавленных
    const double tf_increment = 1./ words.size();
    for(const string& word: words) {
        words_measures_[word][document_id] += tf_increment; // здесь наращиваем text frequency
    }
    documents_.emplace(document_id, DocumentData{ratings, status}); // обновляем количество документов в сервере
}
/**
 * Совпадающие слова в запросе к конкретному документу и статус документа
 */
std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
                                                                                 int document_id) const {
    vector<string> words_matched;
    const Query query_parsed = ParseQuery(raw_query);
    for(const string& word : query_parsed.words_plus) {
        if(words_measures_.count(word) == 0 || words_measures_.at(word).count(document_id) == 0) continue;
        words_matched.push_back(word);
    }
    for(const string& word : query_parsed.words_minus) {
        if(words_measures_.count(word) == 0 || words_measures_.at(word).count(document_id) == 0) continue;
        words_matched.clear();
        break;
    }
    return {words_matched, documents_.at(document_id).status};
}
/**
 * Идентификатор документа по его порядковому индексу
 */
int SearchServer::GetDocumentId(int index) const  {
    if(index < 0) throw out_of_range(Document::ERROR_DOCUMENT_INDEX + " = '"s + to_string(index) + "'"s);
    if(index > GetDocumentCount())
        throw out_of_range(Document::ERROR_DOCUMENT_INDEX + " = "s + to_string(index) + " > "s + to_string(GetDocumentCount()));
    return document_ids_[index];
}
/**
 * Разложить входной текст в вектор из слов, исключая известные стоп-слова
 */
std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const string& word : StringProcessing::SplitIntoWords(text)) {
        if (IsStopWord(word)) continue;
        words.push_back(word);
    }
    return words;
}
/**
 * Получить слово запроса из текста
 */
SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {
    QueryWord qw({text, (text.front() == '-'), IsStopWord(text)});
    if(!qw.is_minus) return qw;
    qw.text.erase(qw.text.begin());
    if(qw.text.empty()) throw invalid_argument(ERROR_MINUS_WORD_EMPTY);
    if(qw.text.front() == '-') throw invalid_argument(ERROR_MINUS_WORD_EXTRADASH + " '"s + qw.text + "'"s);
    return qw;
}
/**
 * Получить структурированный запрос из текста
 */
SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;
    for (const std::string& word : StringProcessing::SplitIntoWords(text)) {
        if(word.empty()) continue;
        const QueryWord query_word = ParseQueryWord(word);
        if (query_word.is_stop) continue; // отсеиваем стоп-слова
        // складываем плюс-слова
        if (!query_word.is_minus) {
            query.words_plus.insert(query_word.text);
            continue;
        }
        // складываем минус-слова
        query.words_minus.insert(query_word.text);
    }
    return query;
}
/**
 * Вычислить IDF для слова
 */
double SearchServer::CalcIdf(const std::string& word) const {
    if(word.empty()) return 0;
    return log(static_cast<double>(GetDocumentCount())/ words_measures_.at(word).size());
}
/**
 * Содержатся ли минус-слова в документе
 */
bool SearchServer::IsDocHasMinus(const std::pair<int, double>& doc,
                                 const std::set<std::string> &words_minus) const {
    for (const string &word : words_minus) {
        if(words_measures_.count(word) == 0) continue;
        for (const auto& measures : words_measures_.at(word)) {
            if(doc.first != measures.first) continue;
            return true;
        }
    }
    return false;
}
