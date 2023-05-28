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

using namespace std;
/**
 * Максимальное число документов в выдаче
 */
const int MAX_RESULT_DOCUMENT_COUNT = 5;
/**
 * Допустимая погрешность вычислений при округлении
 */
const double ERROR_VALUE_EPSILON = 1e-6;
/**
 * Описание ошибки - пустое минус-слово
 */
const char* ERROR_MINUS_WORD_EMPTY = "В запросе содержится пустое минус-слово";
/**
 * Описание ошибки - минус-слово содержит лишнее тире
 */
const char* ERROR_MINUS_WORD_EXTRADASH = "Минус-слово содержит лишнее тире";
/**
 * Описание ошибки - недопустимый код символа
 */
const char* ERROR_INCORRECT_CHAR_WITH_CODE = "Недопустимый код символа";
/**
 * Описание ошибки - недопустимый код символа в слове
 */
const char* ERROR_INCORRECT_WORD = "Недопустимый код символа в слове";
/**
 * Описание ошибки - некорректный идентификатор документа
 */
const char* ERROR_DOCUMENT_ID = "Некорректный идентификатор документа";
/**
 * Описание ошибки - некорректный идентификатор документа
 */
const char* ERROR_DOCUMENT_INDEX = "Некорректный индекс документа";
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
            if(c >= '\0' && c < ' ') throw invalid_argument(ERROR_INCORRECT_CHAR_WITH_CODE + " = '"s + to_string(c) + "'"s);
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
     * Конструктор
     */
    Document() = default;
    /**
     * Конструктор
     */
    Document(int id, double relevance, int rating):
        id(id),
        relevance(relevance),
        rating(rating) { }
    /**
     * Идентификатор
     */
    int id = 0;
    /**
     * Релевантность
     */
    double relevance = 0.0;
    /**
     * Рейтинг
     */
    int rating = 0;
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
     * Значение идентификатора если документ отсуствует
     */
    inline static constexpr int INVALID_DOCUMENT_ID = -1;
    /**
     * Конструктор
     */
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words):
        stop_words_(ToNonEmptySet(stop_words)) { }
    /**
     * Конструктор
     */
    explicit SearchServer(const string& stop_words_text):
        SearchServer(SplitIntoWords(stop_words_text)) { }
    /**
     * Добавить новый документ с id, содержимым, статусом и оценками рейтинга
     */
    void AddDocument(int document_id,
                     const string& document,
                     DocumentStatus status,
                     const vector<int>& ratings) {
        if(document_id < 0 || documents_.count(document_id)) throw invalid_argument(ERROR_DOCUMENT_ID + " = '"s +
                                                                                    to_string(document_id) + "'"s);
        try {
            const vector<string> words = SplitIntoWordsNoStop(document);
            const double tf_increment = 1./ words.size();
            for(const string& word: words) {
                words_measures_[word][document_id] += tf_increment; // здесь наращиваем text frequency
            }
            documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status}); // обновляем количество документов в сервере
        } catch (const invalid_argument& error) { throw error;}
    }
    /**
     * Найти документы, отсортированные по релевантности запросу
     * Вариант с функциональным объектом в качестве параметра
     * Выводит максимум MAX_RESULT_DOCUMENT_COUNT документов
     */
    template<typename Functor>
    vector<Document> FindTopDocuments(const string& raw_query, Functor functor) const {
        Query query;
        try { query = ParseQuery(raw_query); }
        catch (const invalid_argument& error) { throw error; }
        auto matched_documents = FindAllDocuments(query, functor);
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < ERROR_VALUE_EPSILON) {
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
        Query query_parsed;
        try { query_parsed = ParseQuery(raw_query); }  catch (const invalid_argument& error) { throw error; }
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
    int GetDocumentId(int index) const  {
        if(index < 0) throw out_of_range(ERROR_DOCUMENT_INDEX + " = '"s + to_string(index) + "'"s);
        for(const auto& doc : documents_) {
            if(index <= 0) return doc.first;
            --index;
        }
        throw out_of_range(ERROR_DOCUMENT_INDEX + " = "s + to_string(index) + " > "s + to_string(documents_.size()));
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
                if(c >= '\0' && c < ' ') throw invalid_argument(ERROR_INCORRECT_CHAR_WITH_CODE + " = '"s + to_string(c) + "'"s);
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
     * Преобразовать контейнер в набор из непустых слов
     */
    template <typename Container>
    static set<string> ToNonEmptySet(const Container& container) {
        set<string> result;
        for (const string& word : container) {
            if(!IsValidWord(word)) throw invalid_argument(ERROR_INCORRECT_WORD + " = '"s + word + "'"s);
            if (word.empty()) continue;
            result.insert(word);
        }
        return result;
    }
    /**
     * Проверить символы слова на валидность
     */
    static bool IsValidWord(const string& word) {
        return none_of(word.begin(), word.end(),[](const char ch) {
            return ch >= '\0' && ch < ' ';
        });
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
        try {
            for (const string& word : SplitIntoWords(text)) {
                if(word.empty()) continue;
                const QueryWord query_word = ParseQueryWord(word);
                if (query_word.is_stop) continue; // отсеиваем стоп-слова
                // складываем плюс-слова
                if (!query_word.is_minus) {
                    query.words_plus.insert(query_word.text);
                    continue;
                }
                // складываем минус-слова
                if(query_word.text.empty()) throw invalid_argument(ERROR_MINUS_WORD_EMPTY);
                if(query_word.text.front() == '-') throw invalid_argument(ERROR_MINUS_WORD_EXTRADASH + " '"s + query_word.text + "'"s);
                query.words_minus.insert(query_word.text);
            }
        }  catch (const invalid_argument& error) { throw error; }
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
//------------------------------- Удобные функции для ввода-вывода информации------------------------//
/**
 * Вывод в консоль параметров документа
 */
void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}
/**
 * Преобразование статуса документа в строку
 */
string StatusToString(const DocumentStatus status) {
    switch (status) {
        case DocumentStatus::ACTUAL: return "actual"s;
        case DocumentStatus::BANNED: return "banned"s;
        case DocumentStatus::IRRELEVANT: return "irrelevant"s;
        case DocumentStatus::REMOVED: return "removed"s;
        default: return "unknowed"s;
    }
}
/**
 * Вывод результатов поиска
 */
void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << StatusToString(status) << ", "s
         << "words ="s;
    if(!words.empty()) {
        for (const string& word : words) {
            cout << ' ' << word;
        }
    } else cout << " none";
    cout << "}"s << endl;
}
/**
 * Добавление нового документа
 */
void AddDocument(SearchServer& search_server,
                 int document_id,
                 const string& document,
                 DocumentStatus status,
                 const vector<int>& ratings) try {
    search_server.AddDocument(document_id, document, status, ratings);
} catch (const exception& e) {
    cout << "Произошла ошибка при добавлении документа "s << document_id << ": "s << e.what() << endl;
}
/**
 * Поиск документов соответствующих запросу
 */
void FindTopDocuments(const SearchServer& search_server, const string& raw_query) try {
    cout << "Выполняется поиск по запросу: "s << raw_query << endl;
    for (const Document& document : search_server.FindTopDocuments(raw_query)) {
        PrintDocument(document);
    }
} catch (const exception& e) {
    cout << "Произошла ошибка поиска: "s << e.what() << endl;
}
/**
 * Матчинг документов по запросу
 */
void MatchDocuments(const SearchServer& search_server, const string& query) try {
    cout << "Выполняется матчинг документов по запросу: "s << query << endl;
    const int doc_count = search_server.GetDocumentCount();
    for (int index = 0; index < doc_count; ++index) {
        const int doc_id = search_server.GetDocumentId(index);
        const auto [words, status] = search_server.MatchDocument(query, doc_id);
        PrintMatchDocumentResult(doc_id, words, status);
    }
} catch (const exception& e) {
    cout << "Произошла ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
}
//----- Начало работы программы-----//
int main() {
    SearchServer search_server("и в на"s);

    AddDocument(search_server, 1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    AddDocument(search_server, 2, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
    AddDocument(search_server, 5, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
    AddDocument(search_server, 3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    AddDocument(search_server, 4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

    FindTopDocuments(search_server, "пушистый пёс"s);
    FindTopDocuments(search_server, "пушистый --кот"s);
    FindTopDocuments(search_server, "пушистый -"s);

    MatchDocuments(search_server, "пушистый пёс"s);
    MatchDocuments(search_server, "модный -кот"s);
    MatchDocuments(search_server, "модный --пёс"s);
    MatchDocuments(search_server, "пушистый - хвост"s);
    return 0;
}
