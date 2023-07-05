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
 * Пустой контейнер слов и text frequency
 */
const std::map<std::string, double> SearchServer::EMPTY_DOC_MEASURES = {};
/**
 * Начальный итератор загруженных id документов
 */
const std::set<int>::const_iterator SearchServer::begin() const noexcept {
    return document_ids_.begin();
}
/**
 * Конечный итератор загруженных id документов
 */
const std::set<int>::const_iterator SearchServer::end() const noexcept {
    return document_ids_.end();
}
/**
 * Добавить новый документ с id, содержимым, статусом и оценками рейтинга
 */
void SearchServer::AddDocument(int document_id,
                               const std::string& document,
                               DocumentStatus status,
                               const std::vector<int>& ratings) {
    if(document_id < 0 || documents_.count(document_id)) {
        throw invalid_argument(Document::ERROR_DOCUMENT_ID + " = '"s + to_string(document_id) + "'"s);
    }
    const vector<string> words = SplitIntoWordsNoStop(document);
    const double tf_increment = 1./ words.size();
    for(const string& word: words) {
        words_measures_[word][document_id] += tf_increment; // здесь наращиваем text frequency
        document_measures_[document_id][word] += tf_increment;
    }
    documents_.emplace(document_id, DocumentData{ratings, status}); // обновляем количество документов в сервере
    document_ids_.emplace(document_id); // добавляем id документа в список добавленных
}
/**
* Найти документы, отсортированные по релевантности запросу
* Вариант со статусом документа в качестве параметра
* Выводит максимум MAX_RESULT_DOCUMENT_COUNT документов
*/
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query,
                                                     DocumentStatus input_status) const {
    return FindTopDocuments(raw_query, [input_status](int, DocumentStatus status, int) { return status == input_status; });
}
/**
 * Количество загруженных документов
 */
int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}
/**
 * Совпадающие слова в запросе к конкретному документу и статус документа
 */
std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query,
                                                                                 int document_id) const {
    vector<string> words_matched;
    const Query query_parsed = ParseQuery(raw_query);
    for(const string& word : query_parsed.words_plus) {
        if(words_measures_.count(word) == 0 ||
           words_measures_.at(word).count(document_id) == 0) {
            continue;
        }
        words_matched.push_back(word);
    }
    for(const string& word : query_parsed.words_minus) {
        if(words_measures_.count(word) == 0 ||
           words_measures_.at(word).count(document_id) == 0) {
            continue;
        }
        words_matched.clear();
        break;
    }
    return {words_matched, documents_.at(document_id).status};
}
/**
 * Получить text frequency слов по id документа
 */
const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if(document_measures_.count(document_id) == 0) return EMPTY_DOC_MEASURES;
    return document_measures_.at(document_id);
}
/**
 * Получить уникальные слова документа
 */
const std::vector<std::string> SearchServer::GetUniqueWords(int document_id) const {
    const auto &words_tf = GetWordFrequencies(document_id);
    vector<string> words;
    words.reserve(words_tf.size());
    transform(words_tf.begin(), words_tf.end(), back_inserter(words), [](const pair<string, double>& v) {
        return v.first;
    });
    return words;
}
/**
 * Удалить документ по его id
 */
void SearchServer::RemoveDocument(int document_id) {
    if(documents_.count(document_id) == 0 &&
       document_ids_.count(document_id) == 0 &&
       document_measures_.count(document_id) == 0) return;
    // вычищаем измерения документа в словаре
    const auto &doc_measure = document_measures_.at(document_id);
    for(auto &[word, tf] : doc_measure) {
        words_measures_.at(word).erase(document_id);
    }
    // вычищаем данные о документе в остальных переменных
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    document_measures_.erase(document_id);
}
/**
 * Является ли слово стоп-словом
 */
bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}
/**
 * Разложить входной текст в вектор из слов, исключая известные стоп-слова
 */
std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const string& word : StringProcessing::SplitIntoWords(text)) {
        if (IsStopWord(word)) {
            continue;
        }
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
    if(qw.text.empty()) {
        throw invalid_argument(ERROR_MINUS_WORD_EMPTY);
    }
    if(qw.text.front() == '-') {
        throw invalid_argument(ERROR_MINUS_WORD_EXTRADASH + " '"s + qw.text + "'"s);
    }
    return qw;
}
/**
 * Получить структурированный запрос из текста
 */
SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;
    for (const std::string& word : StringProcessing::SplitIntoWords(text)) {
        if(word.empty()) {
            continue;
        }
        const QueryWord query_word = ParseQueryWord(word);
        if (query_word.is_stop) {
            continue; // отсеиваем стоп-слова
        }
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
        if(words_measures_.count(word) == 0) {
            continue;
        }
        for (const auto& measures : words_measures_.at(word)) {
            if(doc.first != measures.first) {
                continue;
            }
            return true;
        }
    }
    return false;
}
