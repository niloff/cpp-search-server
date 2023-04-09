#include <algorithm>
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
 * Содержит id и релевантность запросу
 */
struct Document {
    int id;
    double relevance;
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
     * Добавить новый документ с id и содержимым
     */
    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double tf = 1./ words.size();
        for(const string& word: words) {
            words_measures[word][document_id] += tf; // здесь наращиваем text frequency
        }
        ++document_count_; // обновляем количество документов в сервере
    }
    /**
     * Найти документы, отсортированные по релевантности запросу
     * Выводит только MAX_RESULT_DOCUMENT_COUNT документов
     */
    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);
        // сортируем по релевантности
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
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
     * Измерения для слов загруженных документов
     * Содержит id документов, где они встречаются и text frequency
     */
    map<string, map<int, double>> words_measures;
    /**
     * Известные стоп-слова
     */
    set<string> stop_words_;
    /**
     * Количество загруженных документов
     */
    int document_count_ = 0;
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
        return log(static_cast<double>(document_count_)/ words_measures.at(word).size());
    }
    /**
     * Содержатся ли минус-слова в документе
     */
    bool IsDocHasMinus(const pair<int, double>& doc, const set<string> &words_minus) const {
        for (const string &word : words_minus) {
            if(words_measures.count(word) == 0) continue;
            for (const auto& measures : words_measures.at(word)) {
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
    vector<Document> FindAllDocuments(const Query& query) const {
        vector<Document> matched_documents;
        map<int, double> relevances;
        for (const string &word_plus : query.words_plus) {
            if(words_measures.count(word_plus) == 0) continue;
            const double idf = CalcIdf(word_plus);
            for(const auto& measures : words_measures.at(word_plus)) {
                relevances[measures.first] += measures.second * idf;
            }
        }
        for (const auto& document : relevances) {
            if(IsDocHasMinus(document, query.words_minus)) continue;
            matched_documents.push_back({document.first, document.second});
        }
        return matched_documents;
    }
};
/**
 * Создать поисковой сервер
 * Здесь инициализируем стоп-слова, количество документов и их содержимое
 */
SearchServer CreateSearchServer() {
    // читаем стоп-слова
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    // читаем документы
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }
    return search_server;
}

int main() {
    // создаем поисковой сервер
    const SearchServer search_server = CreateSearchServer();
    // читаем запрос
    const string query = ReadLine();
    // выводим результат поиска
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "s
             << "relevance = "s << relevance << " }"s << endl;
    }
}
