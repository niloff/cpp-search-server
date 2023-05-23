#include <algorithm>
#include <cassert>
#include <numeric>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, hint)
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, hint)
#define RUN_TEST(func) RunTestImpl((func), #func)
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
//
//---------------------------------------------Начало фреймворка для тестирования-------------------------------------//
//

template <typename T, typename U>
ostream& operator<<(ostream& out, const pair<T, U>& container) {
    return out << container.first << ": " << container.second;
}

template <typename T>
void Print(ostream& out, const T& container) {
    bool is_first = true;
    for (const auto& element : container) {
        if (is_first) {
            out << element;
            is_first = false;
        }
        else {
            out << ", "s << element;
        }
    }
}

template <typename T>
ostream& operator<<(ostream& out, const vector<T>& container) {
    out << "["s;
    Print(out, container);
    out << "]"s;
    return out;
}

template <typename T>
ostream& operator<<(ostream& out, const set<T>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

template <typename T, typename U>
ostream& operator<<(ostream& out, const map<T, U>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}
/**
 * Функция-шаблон для вывода диагностической информации
 * Реализация при ложном выражении
 */
void AssertImpl(bool expr, const string& expr_str, const string& file, const string& function,
               unsigned line, const string& hint) {
    if(expr) return;
    cerr << file << "("s << line << ")"s << ": "s << function << ": "s;
    cerr << "ASSERT("s << expr_str << ") "s << "failed.";
    if(!hint.empty()) {
        cerr << " Hint: "s << hint;
    }
    cerr << endl;
    abort();
}
/**
 * Функция-шаблон для вывода диагностической информации
 * Реализация при сравнении переменных
 */
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t == u) return;
    cerr << boolalpha;
    cerr << file << "("s << line << "): "s << func << ": "s;
    cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
    cerr << t << " != "s << u << "."s;
    if (!hint.empty()) {
        cerr << " Hint: "s << hint;
    }
    cerr << endl;
    abort();
}
/**
 * Шаблонная функция для запуска теста
 */
template <typename Func>
void RunTestImpl(const Func& func, const string& func_str) {
    func();
    cerr << func_str << " OK"s << endl;
}

/**
 * Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
 */
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 142;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc = found_docs[0];
        ASSERT_EQUAL(doc.id, doc_id);
    }
    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words are not excluded from the document");
    }
}
/**
 * Тест проверяет, что документы, содержащие минус-слова поискового запроса, не включаются в результаты поиска.
 */
void TestMinusWords() {
    const int doc1_id = 42;
    // Поддержка минус-слов.
    // Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
    SearchServer server;
    server.SetStopWords("in the"s);
    server.AddDocument(doc1_id, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(1002, "dog at the party in city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    auto found_docs = server.FindTopDocuments("city -city"s);
    ASSERT_HINT(found_docs.empty(), "Negative words must be subtracted from the query");
    found_docs = server.FindTopDocuments("-party city cat"s);
    ASSERT_HINT((found_docs.size() == 1 && found_docs.front().id == doc1_id), "Invalid search result");
}
/**
 * Проверка матчинга документов
 * При матчинге документа по поисковому запросу должны быть
 * возвращены все слова из поискового запроса, присутствующие в документе
 * Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов
 */
void TestMatchingDocuments() {
    const int doc1_id = 100500;
    SearchServer server;
    server.SetStopWords("in the at"s);
    server.AddDocument(doc1_id, "dog at the party in city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    {
        const vector<string> expected_result = { "city"s, "party"s };
        const auto [matched_words, matched_status] = server.MatchDocument("cat in city at the party"s, doc1_id);
        ASSERT_EQUAL_HINT(matched_words.size(), expected_result.size(), "Incorrect number of matching words");
        ASSERT_EQUAL_HINT(matched_words, expected_result, "Incorrect matching words");
    }
    // проверяем учет минус слов
    {
        const auto [matched_words, matched_status] = server.MatchDocument("party in city -dog"s, doc1_id);
        ASSERT(matched_words.empty());
    }
}
/**
 * Сортировка найденных документов по релевантности.
 * Возвращаемые при поиске документов результаты
 * должны быть отсортированы в порядке убывания релевантности.
 */
void TestSortByRelevance() {
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.SetStopWords("in the at"s);
    server.AddDocument(1, "we organize the best weddings in Kazan"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(2, "we organize events"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(25, "restoran Safar Hotel in Kazan"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(69, "weddings in Italy"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(126, "photographer in Kazan"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(132, "organize Kazan photo"s, DocumentStatus::ACTUAL, ratings);
    auto found_docs = server.FindTopDocuments("organize weddings in Kazan"s);
    ASSERT_EQUAL(found_docs.size(), 5);
    double setpoint = found_docs.front().relevance;
    for(const auto& doc : found_docs) {
        ASSERT_HINT(doc.relevance <= setpoint, "Incorrect sorting by relevance");
        setpoint = doc.relevance;
    }
}

/**
 * Проверка вычисления рейтинга документов
 */
void TestRatingCalculate() {
    const vector<int> ratings = {2147483640, 2, 2};
    const int rating = accumulate(ratings.begin(), ratings.end(), 0) / ratings.size();
    SearchServer server;
    server.SetStopWords("in the at"s);
    server.AddDocument(1, "we organize the best weddings in Kazan"s, DocumentStatus::ACTUAL, {2147483640, 2, 2});
    auto found_docs = server.FindTopDocuments("organize weddings in Kazan"s);
    ASSERT_EQUAL(found_docs.size(), 1u);
    ASSERT_EQUAL_HINT(found_docs.back().rating, rating, "Incorrect rating calculation result");
}
/**
 * Фильтрация результатов поиска с использованием предиката,
 * задаваемого пользователем.
 * Поиск документов, имеющих заданный статус.
 */
void TestPredicateFilter() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    const int doc3_id = 3;
    search_server.AddDocument(doc3_id, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {10});
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus, int) { return document_id % 2 == 0; })) {
        ASSERT_EQUAL_HINT(document.id % 2, 0, "Wrong predicate");
    }
    // Поиск документа имеющего заданный статус
    {
        const auto docs = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
        ASSERT_EQUAL_HINT(docs.size(), 1, "Incorrect searching with status");
        ASSERT_EQUAL_HINT(docs.front().id, doc3_id, "Wrong status");
    }
    // Поиск по предикату с фильтрацией по статусу
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED &&
                                                                   document_id > 0 && rating > 9; })) {
        ASSERT_EQUAL_HINT(document.id, doc3_id, "Wrong predicate");
    }
}
/**
 * Корректное вычисление релевантности найденных документов.
 */
void TestCalculateRelevance() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::ACTUAL, {9});
    auto found_docs = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT(abs(found_docs[0].relevance - 0.866434) < 1e-6);
    ASSERT(abs(found_docs[1].relevance - 0.231049) < 1e-6);
    ASSERT(abs(found_docs[2].relevance - 0.173287) < 1e-6);
    ASSERT(abs(found_docs[3].relevance - 0.173287) < 1e-6);
}

/**
 * Точка входа для запуска тестов
 */
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // Не забудьте вызывать остальные тесты здесь
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestRatingCalculate);
    RUN_TEST(TestPredicateFilter);
    RUN_TEST(TestCalculateRelevance);
}
//
//-----------------------------------------------------------------------------------------------------------------//
//
/**
 * Начало программы
 */
int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
