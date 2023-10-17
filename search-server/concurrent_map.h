#pragma once
#include <map>
#include <vector>
#include <mutex>
/**
 * Потокобезопасный словарь
 */
template <typename Key, typename Value>
class ConcurrentMap {
private:
    /**
     * Потокобезопасный подсловарь
     */
    struct Bucket {
        /**
         * Мьютекс на доступ к элементам подсловаря
         */
        std::mutex mutex_value;
        /**
         * Подсловарь
         */
        std::map<Key, Value> map;
    };
public:
    // проверка на использование в качестве ключа только целых чисел
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");
    /**
     * Структура для синхронизации доступа к ссылке на значение в словаре
     */
    struct Access {
        /**
         * Конструктор.
         * Захватывает ссылку на значение и блокирует доступ
         */
        Access(const Key& key, Bucket& bucket) :
            guard(bucket.mutex_value),
            ref_to_value(bucket.map[key]) { }
        /**
         * Блокировщик
         */
        std::lock_guard<std::mutex> guard;
        /**
         * Ссылка на значение в словаре
         */
        Value& ref_to_value;
    };
    /**
     * Конструктор.
     * Принимает количество подсловарей,
     * на которые надо разбить всё пространство ключей
     */
    explicit ConcurrentMap(size_t bucket_count) :
        buckets_(bucket_count) {}
    /**
     * Аналог оператора map[]
     */
    Access operator[](const Key& key) {
        const uint64_t index = static_cast<uint64_t>(key) % buckets_.size();
        return {key, buckets_[index]};
    }
    /**
     * Стереть значение в словаре по ключу
     */
    auto Erase(const Key& key) {
        uint64_t tmp_key = static_cast<uint64_t>(key) % buckets_.size();
        std::lock_guard guard(buckets_[tmp_key].mutex_value);
        return buckets_[tmp_key].map.erase(key);
    }
    /**
     * Слить вместе части словаря и вернуть весь словарь целиком.
     * Потокобезопасный метод.
     */
    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> merged_map;
        for(Bucket& b : buckets_) {
            std::lock_guard guard(b.mutex_value);
            merged_map.merge(b.map);
        }
        return merged_map;
    }
private:
    /**
     * Подсловари,
     * на которые разбито всё пространство ключей
     */
    std::vector<Bucket> buckets_;
};
