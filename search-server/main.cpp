#include <algorithm>
#include <numeric>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;
/**
 * Максимальное число документов в выдаче
 */
const int MAX_RESULT_DOCUMENT_COUNT = 5;
/**
 * Возвращает введённую строку
 */
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}
/**
 * Возвращает число из строки
 */
int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}
/**
 * Разложить входной текст в вектор из слов
 */
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if(c != ' ') {
            word += c;
            continue;
        }
        if (word.empty()) continue;
        words.push_back(word);
        word.clear();
    }
    // добавляем последнее слово если оно есть
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}
/**
 * Документ
 */
struct Document {
    /**
     * Идентификатор
     */
    int id;
    /**
     * Релевантность
     */
    double relevance;
    /**
     * Рейтинг
     */
    int rating;
};
/**
 * Текущий статус документа
 */
enum class DocumentStatus {
    /**
     * Актуальный
     */
    ACTUAL,
    /**
     * Не соотвествующий запросу
     */
    IRRELEVANT,
    /**
     * Исключен
     */
    BANNED,
    /**
     * Удален
     */
    REMOVED,
};
/**
 * Поисковой сервер
 */
class SearchServer {
public:
    /**
     * Установка стоп-слов сервера
     */
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }
    /**
     * Добавить новый документ с id, содержимым, статусом и оценками рейтинга
     */
    void AddDocument(int document_id,
                     const string& document,
                     DocumentStatus status,
                     const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double tf_increment = 1./ words.size();
        for(const string& word: words) {
            words_measures_[word][document_id] += tf_increment; // здесь наращиваем text frequency
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status}); // обновляем количество документов в сервере
    }
    /**
     * Найти документы, отсортированные по релевантности запросу
     * Вариант с функциональным объектом в качестве параметра
     * Выводит максимум MAX_RESULT_DOCUMENT_COUNT документов
     */
    template<typename Functor>
    vector<Document> FindTopDocuments(const string& raw_query, Functor functor) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, functor);
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                     return lhs.rating > rhs.rating;
                 }
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    /**
    * Найти документы, отсортированные по релевантности запросу
    * Вариант со статусом документа в качестве параметра
    * Выводит максимум MAX_RESULT_DOCUMENT_COUNT документов
    */
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus input_status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [input_status](int, DocumentStatus status, int) { return status == input_status; });
    }
    /**
     * Количество загруженных документов
     */
    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }
    /**
     * Совпадающие слова в запросе к конкретному документу и статус документа
     */
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
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
private:
    /**
     * Слово из запроса
     */
    struct QueryWord {
        /**
         * Содержимое слова
         */
        string text;
        /**
         * Является ли минус-словом
         */
        bool is_minus;
        /**
         * Является ли стоп-словом
         */
        bool is_stop;
    };
    /**
     * Структурированный запрос
     */
    struct Query {
        /**
         * Плюс-слова
         */
        set<string> words_plus;
        /**
         * Минус-слова
         */
        set<string> words_minus;
    };
    /**
     * Данные документа
     */
    struct DocumentData {
        /**
         * Рейтинг
         */
        int rating;
        /**
         * Статус
         */
        DocumentStatus status;
    };
    /**
     * Измерения для слов загруженных документов
     * Содержит id документов, где они встречаются и text frequency
     */
    map<string, map<int, double>> words_measures_;
    /**
     * Известные стоп-слова
     */
    set<string> stop_words_;
    /**
     * Количество загруженных документов
     */
    map<int, DocumentData> documents_;
    /**
     * Является ли слово стоп-словом
     */
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
    /**
     * Разложить входной текст в вектор из слов, исключая известные стоп-слова
     */
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        string word;
        for (const char c : text) {
            if(c != ' ') {
                word += c;
                continue;
            }
            if (word.empty()) continue;
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
            word.clear();
        }
        // добавляем последнее слово если оно есть и не является стоп-словом
        if (!word.empty() && !IsStopWord(word)) {
            words.push_back(word);
        }
        return words;
    }
    /**
     * Рассчитать средний рейтинг
     */
    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }
    /**
     * Получить слово запроса из текста
     */
    QueryWord ParseQueryWord(string text) const {
        QueryWord qw({text, (text.front() == '-'), IsStopWord(text)});
        if(qw.is_minus) qw.text.erase(qw.text.begin());
        return qw;
    }
    /**
     * Получить структурированный запрос из текста
     */
    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            if(word.empty()) continue;
            const QueryWord query_word = ParseQueryWord(word);
            if (query_word.is_stop) continue; // отсеиваем стоп-слова
            // складываем минус-слова
            if (query_word.is_minus) {
                query.words_minus.insert(query_word.text);
                continue;
            }
            // складываем плюс-слова
            query.words_plus.insert(query_word.text);
        }
        return query;
    }
    /**
     * Вычислить IDF для слова
     */
    double CalcIdf(const string& word) const {
        if(word.empty()) return 0;
        return log(static_cast<double>(GetDocumentCount())/ words_measures_.at(word).size());
    }
    /**
     * Содержатся ли минус-слова в документе
     */
    bool IsDocHasMinus(const pair<int, double>& doc, const set<string> &words_minus) const {
        for (const string &word : words_minus) {
            if(words_measures_.count(word) == 0) continue;
            for (const auto& measures : words_measures_.at(word)) {
                if(doc.first != measures.first) continue;
                return true;
            }
        }
        return false;
    }
    /**
     * Найти все документы, соответствующие запросу
     * Для документов также расчитывается TF-IDF
     */
    template<typename Functor>
    vector<Document> FindAllDocuments(const Query& query, Functor functor) const {
        map<int, double> relevances;
        for (const string &word_plus : query.words_plus) {
            if(words_measures_.count(word_plus) == 0) continue;
            const double idf = CalcIdf(word_plus);
            for(const auto [doc_id, tf] : words_measures_.at(word_plus)) {
                const DocumentData &doc_data = documents_.at(doc_id);
                if(!functor(doc_id, doc_data.status, doc_data.rating)) continue;
                relevances[doc_id] += tf * idf;
            }
        }
        vector<Document> matched_documents;
        for (const auto& [doc_id, relevance] : relevances) {
            if(IsDocHasMinus({doc_id, relevance}, query.words_minus)) continue;
            matched_documents.push_back({doc_id, relevance, documents_.at(doc_id).rating});
        }
        return matched_documents;
    }
};
/**
 * Вывести характеристики документа в консоль
 */
void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}
/**
 * Начало программы
 */
int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus, int) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}
