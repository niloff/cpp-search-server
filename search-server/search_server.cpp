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
const std::map<std::string_view, double> SearchServer::EMPTY_DOC_MEASURES = {};
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
                               string_view document,
                               DocumentStatus status,
                               const std::vector<int>& ratings) {
    if(document_id < 0 || documents_.count(document_id)  || !StringProcessing::IsValidWord(document)) {
        throw invalid_argument(Document::ERROR_DOCUMENT_ID + " = '"s + to_string(document_id) + "'"s);
    }
    const auto& words = SplitIntoWordsNoStop(document);
    const double tf_increment = 1./ words.size();
    for(const auto& word : words) {
        words_measures_[std::string(word)][document_id] += tf_increment; // здесь наращиваем text frequency
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
std::vector<Document> SearchServer::FindTopDocuments(string_view raw_query,
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
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query,
                                                                                 int document_id) const {
    if(document_id < 0 || documents_.count(document_id) == 0) {
        throw out_of_range(Document::ERROR_DOCUMENT_INDEX + " = '"s + to_string(document_id) + "'"s);
    }
    vector<string_view> words_matched;
    const Query& query_parsed = ParseQuery(raw_query);
    // проверяем на наличие минус-слов в документе
    for(const string_view word : query_parsed.words_minus) {
        if(words_measures_.count(std::string(word)) == 0 ||
           words_measures_.at(std::string(word)).count(document_id) == 0) {
            continue;
        }
        return {words_matched, documents_.at(document_id).status};
    }
    // добавляем совпавшие с запросом плюс слова
    words_matched.reserve(query_parsed.words_plus.size());
    for(const string_view word : query_parsed.words_plus) {
        if(words_measures_.count(std::string(word)) == 0 ||
           words_measures_.at(std::string(word)).count(document_id) == 0) {
            continue;
        }
        words_matched.push_back(word);
    }
    return {words_matched, documents_.at(document_id).status};
}
/**
 * Совпадающие слова в запросе к конкретному документу и статус документа.
 * Последовательная реализация.
 */
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&,
                                                                   std::string_view raw_query,
                                                                   int document_id) const {
    return MatchDocument(raw_query, document_id);
}
/**
 * Совпадающие слова в запросе к конкретному документу и статус документа.
 * Многопоточная реализация
 */
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&,
                                                                   std::string_view raw_query,
                                                                   int document_id) const {
    if(document_id < 0 || documents_.count(document_id) == 0) {
        throw out_of_range(Document::ERROR_DOCUMENT_INDEX + " = '"s + to_string(document_id) + "'"s);
    }
    const Query& query_parsed = ParseQueryParallel(raw_query);
    const auto& doc_measures = document_measures_.at(document_id);
    vector<string_view> words_matched;
    // проверяем на наличие минус-слов в документе
    if (any_of(std::execution::par,
               query_parsed.words_minus.begin(),
               query_parsed.words_minus.end(),
               [&doc_measures](const auto word) {
               return doc_measures.count(word);
    })) {
        return { words_matched, documents_.at(document_id).status};
    }
    // добавляем совпавшие с запросом плюс слова
    words_matched.reserve(query_parsed.words_plus.size());
    copy_if(execution::par,
            query_parsed.words_plus.begin(),
            query_parsed.words_plus.end(),
            back_inserter(words_matched),
            [&doc_measures](const auto word) {
        return doc_measures.count(word);
    });
    sort(std::execution::par, words_matched.begin(), words_matched.end());
    auto it = std::unique(words_matched.begin(), words_matched.end());
    words_matched.erase(it, words_matched.end());
    return {words_matched, documents_.at(document_id).status};
}
/**
 * Получить text frequency слов по id документа
 */
const std::map<string_view, double> &SearchServer::GetWordFrequencies(int document_id) const {
    if(document_measures_.count(document_id) == 0) return EMPTY_DOC_MEASURES;
    return document_measures_.at(document_id);
}
/**
 * Получить уникальные слова документа
 */
const std::vector<string_view> SearchServer::GetUniqueWords(int document_id) const {
    const auto &words_tf = GetWordFrequencies(document_id);
    vector<string_view> words;
    words.reserve(words_tf.size());
    transform(words_tf.begin(), words_tf.end(), back_inserter(words), [](const pair<string_view, double>& v) {
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
    for(const auto& [word, tf] : doc_measure) {
        words_measures_.at(std::string(word)).erase(document_id);
    }
    // вычищаем данные о документе в остальных переменных
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    document_measures_.erase(document_id);
}
/**
 * Удалить документ по его id
 * Последовательная реализация
 */
void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    RemoveDocument(document_id);
}
/**
 * Удалить документ по его id
 * Многопоточная реализация
 */
void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    if(documents_.count(document_id) == 0 &&
       document_ids_.count(document_id) == 0 &&
       document_measures_.count(document_id) == 0) return;
    // вычищаем измерения документа в словаре
    const auto& doc_measure = document_measures_.at(document_id);
    std::for_each(std::execution::par,
                  doc_measure.begin(), doc_measure.end(),
                  [this, &document_id] (const auto& pair) {
        words_measures_.at(std::string(pair.first)).erase(document_id);
    });
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    document_measures_.erase(document_id);
}
/**
 * Является ли слово стоп-словом
 */
bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.count(word) > 0;
}
/**
 * Разложить входной текст в вектор из слов, исключая известные стоп-слова
 */
std::vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    std::vector<std::string_view> words;
    for (const auto& word : StringProcessing::SplitIntoWordsView(text)) {
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
SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
    if(!StringProcessing::IsValidWord(text)) {
             throw invalid_argument("");
    }
    QueryWord qw({text, (text.front() == '-'), IsStopWord(text)});
    if(!qw.is_minus) return qw;
    qw.text.remove_prefix(1);
    if(qw.text.empty()) {
        throw invalid_argument(ERROR_MINUS_WORD_EMPTY);
    }
    if(qw.text.front() == '-') {
        throw invalid_argument(ERROR_MINUS_WORD_EXTRADASH + " '"s + qw.text.data() + "'"s);
    }
    if(!StringProcessing::IsValidWord(qw.text)) {
         throw invalid_argument("Error");
    }
    return qw;
}
/**
 * Получить структурированный запрос из текста
 */
SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
    Query query;
    for (const auto word : StringProcessing::SplitIntoWordsView(text)) {
        const QueryWord& query_word = ParseQueryWord(word);
        if (query_word.is_stop) {
            continue; // отсеиваем стоп-слова
        }
        // складываем плюс-слова
        if (!query_word.is_minus) {
            query.words_plus.push_back(query_word.text);
            continue;
        }
        // складываем минус-слова
        query.words_minus.push_back(query_word.text);
    }
    // сортируем
    sort(query.words_minus.begin(), query.words_minus.end());
    sort(query.words_plus.begin(), query.words_plus.end());
    // убираем повторившиеся слова
    auto last_minus = unique(query.words_minus.begin(), query.words_minus.end());
    auto last_plus = unique(query.words_plus.begin(), query.words_plus.end());
    // для минус слов
    size_t unique_size = last_minus - query.words_minus.begin();
    query.words_minus.resize(unique_size);
    // для плюс слов
    unique_size = last_plus - query.words_plus.begin();
    query.words_plus.resize(unique_size);
    return query;
}
/**
 * Получить структурированный запрос из текста.
 * Реализация под многопоточность
 */
SearchServer::Query SearchServer::ParseQueryParallel(std::string_view text) const {
    Query query;
    for (const auto word : StringProcessing::SplitIntoWordsView(text)) {
        const QueryWord& query_word = ParseQueryWord(word);
        if (query_word.is_stop) {
            continue; // отсеиваем стоп-слова
        }
        // складываем плюс-слова
        if (!query_word.is_minus) {
            query.words_plus.push_back(query_word.text);
            continue;
        }
        // складываем минус-слова
        query.words_minus.push_back(query_word.text);
    }
    return query;
}
/**
 * Вычислить IDF для слова
 */
double SearchServer::CalcIdf(const std::string_view word) const {
    if(word.empty()) return 0;
    return log(static_cast<double>(GetDocumentCount())/ words_measures_.at(std::string(word)).size());
}
/**
 * Содержатся ли минус-слова в документе
 */
bool SearchServer::IsDocHasMinus(const std::pair<int, double>& doc,
                                 const std::vector<std::string_view>& words_minus) const {
    for (const auto word : words_minus) {
        if(words_measures_.count(std::string(word)) == 0) {
            continue;
        }
        for (const auto& measures : words_measures_.at(std::string(word))) {
            if(doc.first != measures.first) {
                continue;
            }
            return true;
        }
    }
    return false;
}
