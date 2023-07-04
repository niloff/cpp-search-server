#pragma once
#include <stddef.h>
#include <ostream>
#include <vector>
/**
 * Вспомогательный класс для определения
 * количества элементов на странице
 */
template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) :
        begin_(begin),
        end_(end),
        page_size_(distance(begin, end)) { }
    /**
     * Указатель на первый документ
     */
    Iterator begin() const {
        return begin_;
    }
    /**
     * Указатель на конец
     */
    Iterator end() const {
        return end_;
    }
    /**
     * Вывод в консоль
     */
    friend std::ostream& operator << (std::ostream& out, const IteratorRange& it) {
        for(auto i = it.begin(); i != it.end(); ++i) {
            out << *i;
        }
        return out;
    }
    size_t size() const {
        return page_size_;
    }
private:
    /**
     * Указатель на первый документ
     */
    Iterator begin_;
    /**
     * Указатель на конец
     */
    Iterator end_;
    /**
     * Количество документов на странице
     */
    size_t page_size_;
};
/**
 * Класс для разделения результатов поиска по страницам
 */
template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        while(begin + page_size <= end) {
            pages.push_back(IteratorRange<Iterator>({begin, begin + page_size}));
            advance(begin, page_size);
        }
        if(begin != end) {
            pages.push_back(IteratorRange<Iterator>({begin, end}));
        }
    }
    /**
     * Указатель на первую страницу
     */
    auto begin() const {
        return pages.begin();
    }
    /**
     * Указатель на конец
     */
    auto end() const {
        return pages.end();
    }
private:
    /**
     * Страницы результата поиска
     */
    std::vector<IteratorRange<Iterator>> pages;
};
/**
 * Выполнить разделение результатов поиска по страницам
 */
template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
