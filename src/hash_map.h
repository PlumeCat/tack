// hash_map.h

#pragma once

#include <vector>
#include <cmath>
#include <cstdint>
#include <stdexcept>

/*
    ctor
        init-list [ pair [ k, v ] ]
        size_t buckets
        begin/end iterator [ pair [ k, v ] ],
    dtor
    operator=

    begin/end
    empty
    size
    count
    find
    contains

    at

    insert
    insert_or_assign
    emplace
    try_emplace
    erase
    clear
*/



template<typename ... Bases> struct overload_call_op : Bases ... {
    using is_transparent = void;
    using Bases::operator() ... ;
};
struct char_pointer_hash {
    auto operator()( const char* ptr ) const noexcept {
        return std::hash<std::string_view>{}(ptr);
    }
};
template<typename T> struct custom_hash : public std::hash<T> {};
template<> struct custom_hash<std::string> : overload_call_op<
    std::hash<std::string>,
    std::hash<std::string_view>,
    char_pointer_hash
> {};

template<typename T> struct custom_cmp : public std::equal_to<T> {};
template<> struct custom_cmp<std::string> {
    auto operator()(const std::string& lhs, auto&& rhs) const noexcept {
        return lhs == rhs;
    }
};



template<typename Key, typename Value, typename Hash = custom_hash<Key>, typename Cmp = custom_cmp<Key>>
class hash_map {
    static constexpr uint32_t STATUS_BITS = 0b11 << 30; // top 2 bits
    static constexpr uint32_t HASH_BITS = ~STATUS_BITS; // bottom 30 bits

    static constexpr uint32_t STATUS_FREE = 0u;
    static constexpr uint32_t STATUS_BUSY = 0b10 << 30;
    static constexpr uint32_t STATUS_TOMB = 0b01 << 30;

    static constexpr uint32_t MAX_TOMBSTONE_RATIO = 2;
    static constexpr uint32_t MAX_LOAD_RATIO = 2;
    static constexpr uint32_t MAX_INDEX = 0xffffffff;

    using node_type = std::pair<Key, Value>;
    using storage = std::vector<node_type>;

    mutable Hash hasher;
    mutable Cmp cmp;
    
    uint32_t num_buckets;
    std::vector<uint32_t> s_hash;
    std::vector<uint32_t> locate;

public:
    storage nodes;

public:
    using iterator = typename storage::iterator;
    using const_iterator = typename storage::const_iterator;
    using key_type = Key;
    using value_type = Value;

    hash_map() noexcept: hash_map(8) {}
    explicit hash_map(int buckets) noexcept:
        hasher {},
        cmp {},
        num_buckets { 1u << (int(ceil(log2(std::max(buckets, 8))))) },
        s_hash(num_buckets, STATUS_FREE),
        locate(num_buckets, MAX_INDEX),
        nodes {} {}
    hash_map(std::initializer_list<std::pair<Key, Value>> values): hash_map(values.size()) {
        for (auto& [ k, v ]: values) {
            try_emplace(k, v);
        }
    }
    hash_map(hash_map&&) = default;
    hash_map& operator=(hash_map&&) = default;
    hash_map(const hash_map&) = default;
    hash_map& operator=(const hash_map&) = default;
    
    ~hash_map() {}

    /* WRITE */
    void clear() {
        std::fill(s_hash.begin(), s_hash.end(), STATUS_FREE);
        std::fill(locate.begin(), locate.end(), MAX_INDEX);
        nodes.clear();
    }

    std::pair<iterator, bool> try_emplace(auto&& key, auto&&... args) {
        const auto H = hash(std::forward<decltype(key)>(key));
        const auto slot = linear_probe_insert(H, std::forward<decltype(key)>(key));
        if (slot == MAX_INDEX) {
            reindex(num_buckets * 2);
            return try_emplace(std::forward<decltype(key)>(key), std::forward<decltype(args)>(args)...);
        }

        if (is_busy(s_hash[slot])) {
            return { nodes.begin() + locate[slot], false };
        } else {
            s_hash[slot] = STATUS_BUSY | H;
            locate[slot] = nodes.size();
            nodes.emplace_back(node_type(std::piecewise_construct,
               std::forward_as_tuple(std::forward<decltype(key)>(key)),
               std::forward_as_tuple(std::forward<decltype(args)>(args)...)
            ));
            if (nodes.size() * MAX_LOAD_RATIO > num_buckets) {
                reindex(num_buckets * 2);
            }
            return { nodes.end() - 1, true };
        }
    }

    void insert_or_assign(auto&& key, auto&& value) {
        const auto H = hash(std::forward<decltype(key)>(key));
        const auto slot = linear_probe_insert(H, std::forward<decltype(key)>(key));
        if (slot == MAX_INDEX) {
            reindex(num_buckets * 2);
            return insert_or_assign(std::forward<decltype(key)>(key), std::forward<decltype(value)>(value));
        }

        if (is_busy(s_hash[slot])) {
            nodes[locate[slot]].second = std::forward<decltype(value)>(value);
        } else {
            s_hash[slot] = STATUS_BUSY | H;
            locate[slot] = nodes.size();
            nodes.emplace_back(std::forward<decltype(key)>(key), std::forward<decltype(value)>(value));
            if (nodes.size() * MAX_LOAD_RATIO > num_buckets) {
                reindex(num_buckets * 2); // if the load factor is larger than 0.5, eagerly reindex
            }
        }
    }

    iterator find(auto&& key) {
        const auto H = hash(std::forward<decltype(key)>(key));
        const auto slot = linear_probe(H, std::forward<decltype(key)>(key));
        if (slot != MAX_INDEX && is_busy(s_hash[slot])) {
            return begin() + locate[slot];
        }
        return end();
    }

    const_iterator find(auto&& key) const {
        const auto H = hash(std::forward<decltype(key)>(key));
        const auto slot = linear_probe(H, std::forward<decltype(key)>(key));
        if (slot != MAX_INDEX && is_busy(s_hash[slot])) {
            return begin() + locate[slot];
        }
        return end();
    }

    size_t erase(auto&& key) {
        const auto H = hash(std::forward<decltype(key)>(key));
        const auto slot = linear_probe(H, std::forward<decltype(key)>(key));
        if (slot == MAX_INDEX) {
            return 0;
        }

        const auto sh = s_hash[slot];
        if (is_busy(sh)) {
            const auto ki = locate[slot];
            const auto back = nodes.size() - 1;
            locate[slot] = MAX_INDEX;
            s_hash[slot] = STATUS_TOMB;
            
            if (ki != back) {
                // swap the last stored item into this slot
                nodes[ki] = std::move(nodes.back());

                // update index entry that was pointing at back
                const auto H2 = hash(nodes[ki].first);
                for (auto p = 0u; p < num_buckets; p++) {
                    const auto bi = bucket(H2 + p);
                    if (is_busy(s_hash[bi]) && locate[bi] == back) {
                        locate[bi] = ki;
                        break;
                    }
                }
            }

            nodes.pop_back();
            return 1;
        }

        return 0;
    }

    /* READ */
    const Value& at(auto&& key) const {
        if (const auto node = find(std::forward<decltype(key)>(key)); node != end()) {
            return node->second;
        }
        throw std::runtime_error { "key not found" };
    }
    bool contains(auto&& key) const {
        return find(std::forward<decltype(key)>(key)) != end();
    }

    /* INFO */
    size_t size() const noexcept { return nodes.size(); }
    bool empty() const noexcept { return nodes.size() == 0; }

    /* ITERATE */
    iterator begin() noexcept { return nodes.begin(); }
    iterator end() noexcept { return nodes.end(); }
    const_iterator begin() const noexcept { return nodes.begin(); }
    const_iterator end() const noexcept { return nodes.end(); }
    const_iterator cbegin() const noexcept { return nodes.cbegin(); }
    const_iterator cend() const noexcept { return nodes.cend(); }

private:
    // linear probe from H&N, return slot with key if present or first free slot
    // tombstones are skipped
    // TODO: we could shuffle tombstones forwards???
    uint32_t linear_probe(uint32_t H, auto&& key) const noexcept {
        for (auto p = 0u; p < num_buckets; p++) {
            const auto b = bucket(H + p);
            if (is_free(s_hash[b]) || (s_hash[b] == (STATUS_BUSY | H) && cmp(nodes[locate[b]].first, key))) {
                return b;
            }
        }
        return MAX_INDEX;
    }

    // linear probe from H&N, return slot with key if present or first tombstone or free slot found
    // if scan finds a tombstone it must keep searching for the key, but if not found the tombstone will be used
    uint32_t linear_probe_insert(uint32_t H, auto&& key) const noexcept {
        auto first_tomb = MAX_INDEX;
        for (auto p = 0u; p < num_buckets; p++) {
            const auto b = bucket(H + p);
            if (is_tomb(s_hash[b]) && first_tomb == MAX_INDEX) {
                first_tomb = b;
            } else if (is_free(s_hash[b])) {
                return (first_tomb == MAX_INDEX) ? b : first_tomb;
            } else if (s_hash[b] == (STATUS_BUSY | H) && cmp(nodes[locate[b]].first, key)) {
                return b;
            }
        }
        return first_tomb;
    }

    void reindex(uint32_t new_num_buckets) {
        num_buckets = new_num_buckets;

        auto new_sh = std::vector<uint32_t>(num_buckets, STATUS_FREE);
        auto new_loc = std::vector<uint32_t>(num_buckets, MAX_INDEX);
        for (auto i = 0u; i < (uint32_t)s_hash.size(); i++) {
            if (is_busy(s_hash[i])) {
                for (auto p = 0u; p < num_buckets; p++) {
                    const auto b = bucket((s_hash[i] & HASH_BITS) + p);
                    if (is_free(new_sh[b])) {
                        new_sh[b] = s_hash[i];
                        new_loc[b] = locate[i];
                        break;
                    }
                }
            }
        }

        locate = std::move(new_loc);
        s_hash = std::move(new_sh);
    }

    uint32_t bucket(uint32_t h) const noexcept { return h & (num_buckets - 1); }
    uint32_t hash(auto&& key) const noexcept { return HASH_BITS & uint32_t(hasher(std::forward<decltype(key)>(key)) & 0xfffffffful); }

    bool is_busy(uint32_t s_hash) const noexcept { return (s_hash & STATUS_BITS) == STATUS_BUSY; }
    bool is_tomb(uint32_t s_hash) const noexcept { return (s_hash & STATUS_BITS) == STATUS_TOMB; }
    bool is_free(uint32_t s_hash) const noexcept { return (s_hash & STATUS_BITS) == STATUS_FREE; }
};
