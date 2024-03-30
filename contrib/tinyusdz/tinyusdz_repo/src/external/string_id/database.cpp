// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "database.hpp"

#include <cassert>
#include <cmath>
#include <cstring>
#include <string>

namespace sid = foonathan::string_id;

sid::basic_database::insert_status sid::basic_database::insert_prefix(hash_type hash, hash_type prefix,
                                                                      const char *str, std::size_t length)
{
    std::string prefix_str = lookup(prefix);
    return insert(hash, (prefix_str + str).c_str(), prefix_str.size() + length);
}

namespace
{
    // equivalent to prefix + str == other_str for std::string
    // prefix and other_str are null-terminated
    bool strequal(const char *prefix,
                  const char *str, std::size_t length, const char *other_str) FOONATHAN_NOEXCEPT
    {
        assert(prefix);
        while (*prefix)
            if (*prefix++ != *other_str++)
                return false;
        return std::strncmp(str, other_str, length) == 0;
    }
}

/// \cond impl
class sid::map_database::node_list
{    
    struct node
    {
        std::size_t length; // length of string
        hash_type hash;
        node *next;
        
        node(const char *str, std::size_t length,
             hash_type h, node *next) FOONATHAN_NOEXCEPT
        : length(length), hash(h), next(next)
        {
            void* mem = this;
            auto dest = static_cast<char*>(mem) + sizeof(node);
            std::strncpy(dest, str, length);
            dest[length] = 0;
        }
        
        node(const char *prefix, std::size_t length_prefix,
             const char *str, std::size_t length_string,
             hash_type h, node *next) FOONATHAN_NOEXCEPT
        : length(length_prefix + length_string), hash(h), next(next)
        {
            void* mem = this;
            auto dest = static_cast<char*>(mem) + sizeof(node);
            std::strncpy(dest, prefix, length_prefix);
            dest += length_prefix;
            std::strncpy(dest, str, length_string);
            dest[length_string] = 0;
        }
        
        const char* get_str() const FOONATHAN_NOEXCEPT
        {
            const void *mem = this;
            return static_cast<const char*>(mem) + sizeof(node);
        }
    };
    
    struct insert_pos_t
    {        
        bool exists; // if true: cur set, else: prev and next set
        
        node*& prev;
        node* next;
        
        node* cur;
        
        insert_pos_t(node*& prev, node *next) FOONATHAN_NOEXCEPT
        : exists(false), prev(prev), next(next), cur(nullptr) {}
        
        insert_pos_t(node *cur) FOONATHAN_NOEXCEPT
        : exists(true), prev(this->cur), next(nullptr), cur(cur) {}
    };
    
public:
    node_list() FOONATHAN_NOEXCEPT
    : head_(nullptr) {}
    
    ~node_list() FOONATHAN_NOEXCEPT
    {
        auto cur = head_;
        while (cur)
        {
            auto next = cur->next;
            ::operator delete(cur);
            cur = next;
        }
    }
    
    basic_database::insert_status insert(hash_type hash, const char *str, std::size_t length)
    {
        auto pos = insert_pos(hash);
        if (pos.exists)
            return std::strncmp(str, pos.cur->get_str(), length) == 0 ?
                   basic_database::old_string : basic_database::collision;
        auto mem = ::operator new(sizeof(node) + length + 1);
        auto n = ::new(mem) node(str, length, hash, pos.next);
        pos.prev = n;
        return basic_database::new_string;
    }
    
    basic_database::insert_status insert_prefix(node_list &prefix_bucket, hash_type prefix,
                                                hash_type hash, const char *str, std::size_t length)
    {
        auto prefix_node = prefix_bucket.find_node(prefix);
        auto pos = insert_pos(hash);
        if (pos.exists)
            return strequal(prefix_node->get_str(), str, length, pos.cur->get_str()) ?
                   basic_database::old_string : basic_database::collision;
        auto mem = ::operator new(sizeof(node) + prefix_node->length + length + 1);
        auto n = ::new(mem) node(prefix_node->get_str(), prefix_node->length,
                                 str, length, hash, pos.next);
        pos.prev = n;
        return basic_database::new_string;
    }
    
    // inserts all nodes into new buckets, this list is empty afterwards
    void rehash(node_list *buckets, std::size_t size) FOONATHAN_NOEXCEPT
    {
        auto cur = head_;
        while (cur)
        {
            auto next = cur->next;
            auto pos = buckets[cur->hash % size].insert_pos(cur->hash);
            assert(!pos.exists && "element can't be there already");
            pos.prev = cur;
            cur->next = pos.next;
            cur = next;
        }
        head_ = nullptr;
    }
    
    // returns element with hash, there must be one
    const char* lookup(hash_type h) const FOONATHAN_NOEXCEPT
    {
        return find_node(h)->get_str();
    }
    
private:
    node* find_node(hash_type h) const FOONATHAN_NOEXCEPT
    {
        assert(head_ && "hash not inserted");
        auto cur = head_;
        while (cur->hash < h)
        {
            cur = cur->next;
            assert(cur && "hash not inserted");
        }
        assert(cur->hash == h && "hash not inserted");
        return cur;
    }
    
    insert_pos_t insert_pos(hash_type hash) FOONATHAN_NOEXCEPT
    {
        node *cur = head_, *prev = nullptr;
        while (cur && cur->hash <= hash)
        {
            if (cur->hash < hash)
            {
                prev = cur;
                cur = cur->next;
            }
            else if (cur->hash == hash)
                return {cur};
        }
        return {prev ? prev->next : head_, cur};
    }
    
    node *head_;
};
/// \endcond

sid::map_database::map_database(std::size_t size, double max_load_factor)
: buckets_(new node_list[size]),
  no_items_(0u), no_buckets_(size),
  max_load_factor_(max_load_factor),
  next_resize_(static_cast<std::size_t>(std::floor(no_buckets_ * max_load_factor_)))
{}

sid::map_database::~map_database() FOONATHAN_NOEXCEPT {}

sid::basic_database::insert_status sid::map_database::insert(hash_type hash, const char *str, std::size_t length)
{
    if (no_items_ + 1 >= next_resize_)
        rehash();
    auto status = buckets_[hash % no_buckets_].insert(hash, str, length);
    if (status == insert_status::new_string)
        ++no_items_;
    return status;
}

sid::basic_database::insert_status sid::map_database::insert_prefix(hash_type hash, hash_type prefix,
                                                                    const char *str, std::size_t length)
{
    if (no_items_ + 1 >= next_resize_)
        rehash();
    auto status = buckets_[hash % no_buckets_].insert_prefix(buckets_[prefix % no_buckets_], prefix,
                                                             hash, str, length);
    if (status == insert_status::new_string)
        ++no_items_;
    return status;
}

const char* sid::map_database::lookup(hash_type hash) const FOONATHAN_NOEXCEPT
{
    return buckets_[hash % no_buckets_].lookup(hash);
}

void sid::map_database::rehash()
{
    static FOONATHAN_CONSTEXPR auto growth_factor = 2;
    auto new_size = growth_factor * no_buckets_;
    auto buckets = new node_list[new_size]();
    auto end = buckets_.get() + no_buckets_;
    for (auto list = buckets_.get(); list != end; ++list)
        list->rehash(buckets, new_size);
    buckets_.reset(buckets);
    no_buckets_ = new_size;
    next_resize_ = static_cast<std::size_t>(std::floor(no_buckets_ * max_load_factor_));
}
