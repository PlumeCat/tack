#pragma once

#include <vector>
#include <string>
#include <utility>

// Adapted from khash (commit: 5fc2090) to have a more C++-friendly 
// Uses std::vector instead of realloc

/* The MIT License

   Copyright (c) 2008, 2009, 2011 by Attractive Chaos <attractor@live.co.uk>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#define __ac_isempty(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&2)
#define __ac_isdel(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&1)
#define __ac_iseither(flag, i) ((flag[i>>4]>>((i&0xfU)<<1))&3)
#define __ac_set_isdel_false(flag, i) (flag[i>>4]&=~(1ul<<((i&0xfU)<<1)))
#define __ac_set_isempty_false(flag, i) (flag[i>>4]&=~(2ul<<((i&0xfU)<<1)))
#define __ac_set_isboth_false(flag, i) (flag[i>>4]&=~(3ul<<((i&0xfU)<<1)))
#define __ac_set_isdel_true(flag, i) (flag[i>>4]|=1ul<<((i&0xfU)<<1))
#define __ac_fsize(m) ((m) < 16? 1 : (m)>>4)

#ifndef kroundup32
#define kroundup32(x) (--(x), (x)|=(x)>>1, (x)|=(x)>>2, (x)|=(x)>>4, (x)|=(x)>>8, (x)|=(x)>>16, ++(x))
#endif

static const double __ac_HASH_UPPER = 0.77;

static inline uint32_t hash_string(const std::string& s) {
    if (!s.size()) {
        return 0;
    }
    uint32_t h = s[0];
    for (auto c : s) {
        h = (h << 5) - h + uint32_t(c);
    }
    return h;
}

template<typename KeyType,typename ValType>
struct KHash {
    using Iterator = uint32_t;
    
private:    
    uint32_t n_buckets;
    uint32_t size_;
    uint32_t n_occupied;
    uint32_t upper_bound;
    std::vector<uint32_t> flags;
    std::vector<KeyType> keys;
    std::vector<ValType> vals;
    
    // using HashFunc = std::hash<std::string>;
    // HashFunc hash_func;
    
    #define hash_func(s) hash_string(s)
    #define cmp_func(a, b) ((a) == (b))

public:
    KHash(): n_buckets(0), size_(0), n_occupied(0), upper_bound(0) {}
    ~KHash() {}
    KHash(const KHash&) = delete;
    KHash& operator=(const KHash&) = delete;
    KHash(KHash&&) = delete;
    KHash& operator=(KHash&&) = delete;

    void clear() {
        if (flags.size()) {
            flags.assign(flags.size(), 0xaaaaaaaa);
            size_ = 0;
            n_occupied = 0;
        }
    }

    Iterator get(const KeyType& key) const {
        if (n_buckets) {
            uint32_t step = 0;
            uint32_t mask = n_buckets - 1;
            uint32_t k = hash_func(key);
            uint32_t i = k & mask;
            uint32_t last = i;
            while (!__ac_isempty(flags, i) && (__ac_isdel(flags, i) || !cmp_func(keys[i], key))) {
                i = (i + (++step)) & mask;
                if (i == last) {
                    return n_buckets;
                }
            }
            return __ac_iseither(flags, i) ? n_buckets : i;
        } else {
            return 0;
        }
    }

    int resize(uint32_t new_n_buckets) {
        auto rehash_needed = true;
        auto new_flags = std::vector<uint32_t>{};

        kroundup32(new_n_buckets);
        if (new_n_buckets < 4) {
            new_n_buckets = 4;
        }
        if (size_ >= uint32_t(new_n_buckets * __ac_HASH_UPPER + 0.5)) {
            // requested size is too small
            rehash_needed = false;
        } else {
            // hash table size to be changed (shrink or expand); must rehash
            new_flags = std::vector<uint32_t>(__ac_fsize(new_n_buckets), 0xaaaaaaaa);
            if (n_buckets < new_n_buckets) {
                // expand
                keys.resize(new_n_buckets);
                vals.resize(new_n_buckets);
            }
        }

        if (rehash_needed) {
            for (auto j = 0; j != n_buckets; ++j) {
                if (__ac_iseither(flags, j) == 0) {
                    auto key = keys[j];
                    auto new_mask = new_n_buckets - 1;
                    auto val = vals[j];
                    __ac_set_isdel_true(flags, j);

                    while (true) {
                        uint32_t k = hash_func(key);
                        uint32_t i = k & new_mask;
                        uint32_t step = 0;

                        while (!__ac_isempty(new_flags, i)) {
                            i = (i + (++step)) & new_mask;
                        }

                        __ac_set_isempty_false(new_flags, i);

                        if (i < n_buckets && __ac_iseither(flags, i) == 0) {
                            std::swap(keys[i], key);
                            std::swap(vals[i], val);
                            __ac_set_isdel_true(flags, i);
                        } else {
                            keys[i] = key;
                            vals[i] = val;
                            break;
                        }
                    }
                }
            }
            if (n_buckets > new_n_buckets) {
                keys.resize(new_n_buckets);
                vals.resize(new_n_buckets);
            }

            flags = std::move(new_flags);
            n_buckets = new_n_buckets;
            n_occupied = size_;
            upper_bound = uint32_t(n_buckets * __ac_HASH_UPPER + 0.5);
        }

        return 0;
    }

    Iterator put(const KeyType& key, int* ret) {
        if (n_occupied >= upper_bound) {
            if (n_buckets > (size_ << 1)) {
                if (resize(n_buckets - 1) < 0) {
                    *ret = -1;
                    return n_buckets;
                }
            } else if (resize(n_buckets + 1) < 0) {
                *ret = -1;
                return n_buckets;
            }
        } 


        uint32_t step = 0;
        uint32_t mask = n_buckets - 1;
        uint32_t k = hash_func(key);
        uint32_t i = k & mask;
        uint32_t x = n_buckets;
        uint32_t site = n_buckets;
        if (__ac_isempty(flags, i)) {
            x = i;
        } else {
            uint32_t last = i;
            while (!__ac_isempty(flags, i) && (__ac_isdel(flags, i) || !cmp_func(keys[i], key))) {
                if (__ac_isdel(flags, i)) {
                    site = i;
                }
                i = (i + (++step)) & mask;
                if (i == last) {
                    x = site;
                    break;
                }
            }
            if (x == n_buckets) {
                if (__ac_isempty(flags, i) && site != n_buckets) {
                    x = site;
                } else {
                    x = i;
                }
            }
        }

        if (__ac_isempty(flags, x)) {
            keys[x] = key;
            __ac_set_isboth_false(flags, x);
            ++size_;
            ++n_occupied;
            *ret = 1;
        } else if (__ac_isdel(flags, x)) {
            keys[x] = key;
            ++size_;
            *ret = 2;
        } else {
            *ret = 0;
        }
        
        return x;
    }

    void del(Iterator x) {
        if (x != n_buckets && !__ac_iseither(flags, x)) {
            __ac_set_isdel_true(flags, x);
            --size;
        }
    }

    bool exist(Iterator x) const {
        return !__ac_iseither(flags, x);
    }

    KeyType& key_at(Iterator x) { return keys[x]; }
    const KeyType& key_at(Iterator x) const { return keys[x]; }
    ValType& value_at(Iterator x) { return vals[x]; }
    const ValType& value_at(Iterator x) const { return vals[x]; }

    Iterator begin() const { return 0; }
    Iterator end() const { return n_buckets; }
    uint32_t size() const { return size; }
};
