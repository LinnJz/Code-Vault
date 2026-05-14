/*
 * @lc app=leetcode.cn id=146 lang=cpp
 *
 * [146] LRU 缓存
 */

// @lc code=start
class LRUCache
{
public:
  LRUCache(size_t capacity)
      : m_Capacity { capacity }
      , m_PoolResource(std::pmr::pool_options { 16, sizeof(std::pmr::unordered_map<int, decltype(m_List.begin())>) })
      , m_List(&m_PoolResource)
      , m_CacheMap(&m_PoolResource)
  {
  }

  int get(int key)
  {
    auto findIt = m_CacheMap.find(key);
    if (findIt == m_CacheMap.end())
      return -1;

    m_List.splice(m_List.begin(), m_List, findIt->second);

    return findIt->second->val;
  }

  void put(int key, int value)
  {
    if (auto findIt = m_CacheMap.find(key); findIt != m_CacheMap.end())
    {
      findIt->second->val = value;
      m_List.splice(m_List.begin(), m_List, findIt->second);

      return;
    }
    if (m_List.size() >= m_Capacity) [[unlikely]]
    {
      m_CacheMap.erase(m_List.back().key);
      m_List.pop_back();
    }
    m_List.emplace_front(key, value);
    m_CacheMap.try_emplace(key, m_List.begin());
  }

private:
  struct Node
  {
    int key;
    int val;
  };

  size_t                                                 m_Capacity { 0 };
  std::pmr::unsynchronized_pool_resource                 m_PoolResource;
  std::pmr::list<Node>                                   m_List;
  std::pmr::unordered_map<int, decltype(m_List.begin())> m_CacheMap;
};

// @lc code=end
class LRUCache {
public:
    LRUCache(size_t capacity) 
        : m_Capacity(capacity)
        , m_Nodes(std::make_unique_for_overwrite<Node[]>(capacity))
        , m_Size(0)
        , m_Head(-1)
        , m_Tail(-1)
        , m_FreeHead(0)
    {
        // 初始化空闲链表：所有节点串在一起，索引从0到capacity-1
        for (size_t i = 0; i < capacity - 1; ++i) {
            m_Nodes[i].next = static_cast<int>(i + 1);
        }
        m_Nodes[capacity - 1].next = -1; // 最后一个节点无下一个空闲节点
    }

    int get(int key) {
        auto it = m_KeyToIdx.find(key);
        if (it == m_KeyToIdx.end()) return -1;

        int idx = it->second;
        // 移动到头部
        moveToHead(idx);
        return m_Nodes[idx].val;
    }

    void put(int key, int value) {
        auto it = m_KeyToIdx.find(key);
        if (it != m_KeyToIdx.end()) {
            // 已存在：更新值并移动到头部
            int idx = it->second;
            m_Nodes[idx].val = value;
            moveToHead(idx);
            return;
        }

        // 新节点：尝试分配空闲节点
        int newIdx = allocateNode();
        if (newIdx == -1) {
            // 没有空闲节点，淘汰尾部节点
            newIdx = m_Tail;
            m_KeyToIdx.erase(m_Nodes[newIdx].key);
            removeNode(newIdx);   // 从链表中摘下
        }
        // 复用/新节点赋值
        m_Nodes[newIdx].key = key;
        m_Nodes[newIdx].val = value;
        m_KeyToIdx[key] = newIdx;
        // 插入到头部
        addToHead(newIdx);
    }

private:
    struct Node {
        int key = 0;
        int val = 0;
        int prev = -1;
        int next = -1;
    };

    size_t m_Capacity;
    std::unique_ptr<Node[]> m_Nodes;            // 连续存储，缓存友好
    std::unordered_map<int, int> m_KeyToIdx; // key -> node index
    int m_Head, m_Tail;
    int m_FreeHead;   // 空闲链表头索引，-1表示无空闲节点
    size_t m_Size;    // 已使用节点数（可选，仅用于断言）

    // 从链表中移除节点（不释放资源，仅改前后指针）
    void removeNode(int idx) {
        Node& node = m_Nodes[idx];
        if (node.prev != -1) m_Nodes[node.prev].next = node.next;
        else m_Head = node.next;
        if (node.next != -1) m_Nodes[node.next].prev = node.prev;
        else m_Tail = node.prev;
    }

    // 将节点插入到链表头部
    void addToHead(int idx) {
        Node& node = m_Nodes[idx];
        node.prev = -1;
        node.next = m_Head;
        if (m_Head != -1) m_Nodes[m_Head].prev = idx;
        m_Head = idx;
        if (m_Tail == -1) m_Tail = idx;
    }

    // 移动到头部（相当于先移除再加到头部）
    void moveToHead(int idx) {
        if (m_Head == idx) return;
        removeNode(idx);
        addToHead(idx);
    }

    // 分配新节点：从空闲链表取一个，若无空闲则返回-1
    int allocateNode() {
        if (m_FreeHead == -1) return -1;
        int idx = m_FreeHead;
        m_FreeHead = m_Nodes[idx].next;
        m_Nodes[idx].next = -1;  // 重置，后续addToHead会设置
        return idx;
    }

    // 可选：释放节点回空闲链表（本例未显式使用，因为淘汰时直接复用淘汰的节点）
    // 但为了完整性，这里实现一个，本类中不需要调用
    void freeNode(int idx) {
        m_Nodes[idx].next = m_FreeHead;
        m_FreeHead = idx;
    }
};
#include <list>
#include <optional>
#include <absl/container/flat_hash_map.h>

template <typename Key, typename Value>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}
    
    // 从缓存中获取值，如果存在则移动到最近使用的位置
    std::optional<Value> get(const Key& key) {
        auto it = cache_map_.find(key);
        if (it == cache_map_.end()) {
            return std::nullopt; // 未找到
        }
        
        // 移动到最近使用的位置
        items_list_.splice(items_list_.begin(), items_list_, it->second);
        return it->second->value;
    }
    
    // 将键值对放入缓存
    void put(const Key& key, const Value& value) {
        auto it = cache_map_.find(key);
        if (it != cache_map_.end()) {
            // 键已存在，更新值并移动到最近使用的位置
            it->second->value = value;
            items_list_.splice(items_list_.begin(), items_list_, it->second);
            return;
        }
        
        // 如果缓存已满，移除最久未使用的项
        if (items_list_.size() >= capacity_) {
            auto& last = items_list_.back();
            cache_map_.erase(last.key);
            items_list_.pop_back();
        }
        
        // 添加新项到最近使用的位置
        items_list_.push_front({key, value});
        cache_map_.try_emplace(key, items_list_.begin());
    }
    
    // 检查键是否存在于缓存中
    bool contains(const Key& key) const {
        return cache_map_.find(key) != cache_map_.end();
    }
    
    // 获取当前缓存大小
    size_t size() const {
        return items_list_.size();
    }
    
    // 获取缓存容量
    size_t capacity() const {
        return capacity_;
    }
    
    // 清空缓存
    void clear() {
        items_list_.clear();
        cache_map_.clear();
    }

private:
    struct Item {
        Key key;
        Value value;
    };
    
    size_t capacity_;
    std::list<Item> items_list_;
    absl::flat_hash_map<Key, typename std::list<Item>::iterator> cache_map_;
};
#include <unordered_map>
#include <list>
#include <optional>

template <typename Key, typename Value>
class LFUCache {
private:
    struct Node {
        Key key;
        Value value;
        int frequency;
        Node(Key k, Value v, int freq) : key(k), value(v), frequency(freq) {}
    };

    // 频率到节点的映射，每个频率对应一个双向链表（最近访问的在链表头部）
    std::unordered_map<int, std::list<Node>> freq_map_;
    
    // 键到节点位置的映射
    std::unordered_map<Key, typename std::list<Node>::iterator> key_map_;
    
    size_t capacity_;
    int min_frequency_;  // 当前最小频率

public:
    explicit LFUCache(size_t capacity) : capacity_(capacity), min_frequency_(0) {}
    
    // 从缓存中获取值
    std::optional<Value> get(const Key& key) {
        if (capacity_ == 0) return std::nullopt;
        
        auto it = key_map_.find(key);
        if (it == key_map_.end()) {
            return std::nullopt;
        }
        
        // 获取节点迭代器
        auto node_it = it->second;
        Value value = node_it->value;
        int freq = node_it->frequency;
        
        // 从当前频率链表中移除
        freq_map_[freq].erase(node_it);
        
        // 如果当前频率链表为空且是最小频率，更新最小频率
        if (freq_map_[freq].empty()) {
            freq_map_.erase(freq);
            if (min_frequency_ == freq) {
                min_frequency_ = freq + 1;
            }
        }
        
        // 插入到更高频率的链表头部
        freq_map_[freq + 1].push_front(Node(key, value, freq + 1));
        key_map_[key] = freq_map_[freq + 1].begin();
        
        return value;
    }
    
    // 将键值对放入缓存
    void put(const Key& key, const Value& value) {
        if (capacity_ == 0) return;
        
        auto it = key_map_.find(key);
        if (it != key_map_.end()) {
            // 键已存在，更新值并增加频率
            auto node_it = it->second;
            int freq = node_it->frequency;
            
            // 从当前频率链表中移除
            freq_map_[freq].erase(node_it);
            
            // 如果当前频率链表为空且是最小频率，更新最小频率
            if (freq_map_[freq].empty()) {
                freq_map_.erase(freq);
                if (min_frequency_ == freq) {
                    min_frequency_ = freq + 1;
                }
            }
            
            // 插入到更高频率的链表头部
            freq_map_[freq + 1].push_front(Node(key, value, freq + 1));
            key_map_[key] = freq_map_[freq + 1].begin();
            return;
        }
        
        // 新键，需要检查容量
        if (key_map_.size() >= capacity_) {
            // 移除最小频率链表中的最后一个节点（最久未访问的）
            auto& min_freq_list = freq_map_[min_frequency_];
            Key key_to_remove = min_freq_list.back().key;
            min_freq_list.pop_back();
            key_map_.erase(key_to_remove);
            
            // 如果最小频率链表为空，清理
            if (min_freq_list.empty()) {
                freq_map_.erase(min_frequency_);
            }
        }
        
        // 插入新节点到频率1的链表头部
        min_frequency_ = 1;
        freq_map_[min_frequency_].push_front(Node(key, value, min_frequency_));
        key_map_[key] = freq_map_[min_frequency_].begin();
    }
    
    // 检查键是否存在于缓存中
    bool contains(const Key& key) const {
        return key_map_.find(key) != key_map_.end();
    }
    
    // 获取当前缓存大小
    size_t size() const {
        return key_map_.size();
    }
    
    // 获取缓存容量
    size_t capacity() const {
        return capacity_;
    }
    
    // 清空缓存
    void clear() {
        freq_map_.clear();
        key_map_.clear();
        min_frequency_ = 0;
    }
    
    // 获取键的当前访问频率（用于调试）
    std::optional<int> getFrequency(const Key& key) const {
        auto it = key_map_.find(key);
        if (it == key_map_.end()) {
            return std::nullopt;
        }
        return it->second->frequency;
    }
    
    // 获取当前最小频率（用于调试）
    int getMinFrequency() const {
        return min_frequency_;
    }
};
/**
 * Your LRUCache object will be instantiated and called as such:
 * LRUCache* obj = new LRUCache(capacity);
 * int param_1 = obj->get(key);
 * obj->put(key,value);
 */
// @lc code=end

/*
 * @lc app=leetcode.cn id=146 lang=cpp
 *
 * [146] LRU 缓存
 */

// @lc code=start
class LRUCache {
public:
    explicit LRUCache(int capacity) : capacity_(capacity) {}

    int get(int key) {
        auto it = cache_map_.find(key);
        if (it == cache_map_.end()) {
            return -1;
        }
        // 将访问的节点移动到链表头部
        cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
        return it->second->second;
    }

    void put(int key, int value) {
        auto it = cache_map_.find(key);
        if (it != cache_map_.end()) {
            // 更新已存在键的值，并移动到头部
            it->second->second = value;
            cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
            return;
        }

        // 容量已满，删除LRU元素（链表尾部）
        if (cache_map_.size() >= capacity_) {
            auto& lru_node = cache_list_.back();
            cache_map_.erase(lru_node.first);
            cache_list_.pop_back();
        }

        // 插入新节点到链表头部，并更新哈希表
        cache_list_.emplace_front(key, value);
        cache_map_.try_emplace(key, cache_list_.begin());
    }

private:
    int capacity_;
    std::list<std::pair<int, int>> cache_list_;
    std::unordered_map<int, std::list<std::pair<int, int>>::iterator> cache_map_;
};

/**
 * Your LRUCache object will be instantiated and called as such:
 * LRUCache* obj = new LRUCache(capacity);
 * int param_1 = obj->get(key);
 * obj->put(key,value);
 */

/*
 * LRUCache11 - a templated C++11 based LRU cache class that allows
 * specification of
 * key, value and optionally the map container type (defaults to
 * std::unordered_map)
 * By using the std::unordered_map and a linked list of keys it allows O(1) insert, delete
 * and
 * refresh operations.
 *
 * This is a header-only library and all you need is the LRUCache11.hpp file
 *
 * Github: https://github.com/mohaps/lrucache11
 *
 * This is a follow-up to the LRUCache project -
 * https://github.com/mohaps/lrucache
 *
 * Copyright (c) 2012-22 SAURAV MOHAPATRA <mohaps@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#pragma once
#include <algorithm>
#include <cstdint>
#include <list>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>

namespace lru11 {
    /*
     * a noop lockable concept that can be used in place of std::mutex
     */
    class NullLock {
    public:
        void lock() {}
        void unlock() {}
        bool try_lock() { return true; }
    };

    /**
     * error raised when a key not in cache is passed to get()
     */
    class KeyNotFound : public std::invalid_argument {
    public:
        KeyNotFound() : std::invalid_argument("key_not_found") {}
    };

    template <typename K, typename V>
    struct KeyValuePair {
    public:
        K key;
        V value;

        KeyValuePair(K k, V v) : key(std::move(k)), value(std::move(v)) {}
    };

    /**
     *	The LRU LRUCache class templated by
     *		Key - key type
     *		Value - value type
     *		MapType - an associative container like std::unordered_map
     *		LockType - a lock type derived from the Lock class (default:
     *NullLock = no synchronization)
     *
     *	The default NullLock based template is not thread-safe, however passing
     *Lock=std::mutex will make it
     *	thread-safe
     */
    template <class Key, class Value, class Lock = NullLock,
        class Map = std::unordered_map<
        Key, typename std::list<KeyValuePair<Key, Value>>::iterator>>
        class LRUCache {
        public:
            typedef KeyValuePair<Key, Value> node_type;
            typedef std::list<KeyValuePair<Key, Value>> list_type;
            typedef Map map_type;
            typedef Lock lock_type;
            using Guard = std::lock_guard<lock_type>;
            /**
             * the maxSize is the soft limit of keys and (maxSize + elasticity) is the
             * hard limit
             * the cache is allowed to grow till (maxSize + elasticity) and is pruned back
             * to maxSize keys
             * set maxSize = 0 for an unbounded cache (but in that case, you're better off
             * using a std::unordered_map
             * directly anyway! :)
             */
            explicit LRUCache(size_t maxSize = 64, size_t elasticity = 10)
                : maxSize_(maxSize), elasticity_(elasticity) {
            }
            virtual ~LRUCache() = default;
            size_t size() const {
                Guard g(lock_);
                return cache_.size();
            }
            bool empty() const {
                Guard g(lock_);
                return cache_.empty();
            }
            void clear() {
                Guard g(lock_);
                cache_.clear();
                keys_.clear();
            }
            void insert(const Key& k, Value v) {
                Guard g(lock_);
                const auto iter = cache_.find(k);
                if (iter != cache_.end()) {
                    iter->second->value = v;
                    keys_.splice(keys_.begin(), keys_, iter->second);
                    return;
                }

                keys_.emplace_front(k, std::move(v));
                cache_[k] = keys_.begin();
                prune();
            }
            void emplace(const Key& k, Value&& v) {
                Guard g(lock_);
                keys_.emplace_front(k, std::move(v));
                cache_[k] = keys_.begin();
                prune();
            }
            /**
              for backward compatibity. redirects to tryGetCopy()
             */
            bool tryGet(const Key& kIn, Value& vOut) {
                return tryGetCopy(kIn, vOut);
            }

            bool tryGetCopy(const Key& kIn, Value& vOut) {
                Guard g(lock_);
                Value tmp;
                if (!tryGetRef_nolock(kIn, tmp)) { return false; }
                vOut = tmp;
                return true;
            }

            bool tryGetRef(const Key& kIn, Value& vOut) {
                Guard g(lock_);
                return tryGetRef_nolock(kIn, vOut);
            }
            /**
             *	The const reference returned here is only
             *    guaranteed to be valid till the next insert/delete
             *  in multi-threaded apps use getCopy() to be threadsafe
             */
            const Value& getRef(const Key& k) {
                Guard g(lock_);
                return get_nolock(k);
            }

            /**
                added for backward compatibility
             */
            Value get(const Key& k) {
                return getCopy(k);
            }
            /**
             * returns a copy of the stored object (if found)
             * safe to use/recommended in multi-threaded apps
             */
            Value getCopy(const Key& k) {
                Guard g(lock_);
                return get_nolock(k);
            }

            bool remove(const Key& k) {
                Guard g(lock_);
                auto iter = cache_.find(k);
                if (iter == cache_.end()) {
                    return false;
                }
                keys_.erase(iter->second);
                cache_.erase(iter);
                return true;
            }
            bool contains(const Key& k) const {
                Guard g(lock_);
                return cache_.find(k) != cache_.end();
            }

            size_t getMaxSize() const { return maxSize_; }
            size_t getElasticity() const { return elasticity_; }
            size_t getMaxAllowedSize() const { return maxSize_ + elasticity_; }
            template <typename F>
            void cwalk(F& f) const {
                Guard g(lock_);
                std::for_each(keys_.begin(), keys_.end(), f);
            }

        protected:
            const Value& get_nolock(const Key& k) {
                const auto iter = cache_.find(k);
                if (iter == cache_.end()) {
                    throw KeyNotFound();
                }
                keys_.splice(keys_.begin(), keys_, iter->second);
                return iter->second->value;
            }
            bool tryGetRef_nolock(const Key& kIn, Value& vOut) {
                const auto iter = cache_.find(kIn);
                if (iter == cache_.end()) {
                    return false;
                }
                keys_.splice(keys_.begin(), keys_, iter->second);
                vOut = iter->second->value;
                return true;
            }
            size_t prune() {
                size_t maxAllowed = maxSize_ + elasticity_;
                if (maxSize_ == 0 || cache_.size() < maxAllowed) {
                    return 0;
                }
                size_t count = 0;
                while (cache_.size() > maxSize_) {
                    cache_.erase(keys_.back().key);
                    keys_.pop_back();
                    ++count;
                }
                return count;
            }

        private:
            // Disallow copying.
            LRUCache(const LRUCache&) = delete;
            LRUCache& operator=(const LRUCache&) = delete;

            mutable Lock lock_;
            Map cache_;
            list_type keys_;
            size_t maxSize_;
            size_t elasticity_;
    };

}  // namespace LRUCache11
