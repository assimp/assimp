// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_STRING_ID_DATABASE_HPP_INCLUDED
#define FOONATHAN_STRING_ID_DATABASE_HPP_INCLUDED

#include <memory>

#ifdef __wasi__
#else
#include <mutex>
#endif

#include "basic_database.hpp"
#include "config.hpp"

namespace foonathan { namespace string_id
{
    /// \brief A database that doesn't store the string-values.
    /// \detail It does not detect collisions or allows retrieving,
    /// \c lookup() returns "string_id database disabled".
    class dummy_database : public basic_database
    {
    public:
        insert_status insert(hash_type, const char *, std::size_t) FOONATHAN_OVERRIDE
        {
            return new_string;
        }

        insert_status insert_prefix(hash_type, hash_type, const char *, std::size_t) FOONATHAN_OVERRIDE
        {
            return new_string;
        }

        const char* lookup(hash_type) const FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE
        {
            return "string_id database disabled";
        }
    };

    /// \brief A database that uses a highly optimized hash table.
    class map_database : public basic_database
    {
    public:
        /// \brief Creates a new database with given number of buckets and maximum load factor.
        explicit map_database(std::size_t size = 1024, double max_load_factor = 1.0);
        ~map_database() FOONATHAN_NOEXCEPT;

        insert_status insert(hash_type hash, const char *str, std::size_t length) FOONATHAN_OVERRIDE;
        insert_status insert_prefix(hash_type hash, hash_type prefix,
                                    const char *str, std::size_t length) FOONATHAN_OVERRIDE;
        const char* lookup(hash_type hash) const FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE;

    private:
        void rehash();

        class node_list;
        std::unique_ptr<node_list[]> buckets_;
        std::size_t no_items_, no_buckets_;
        double max_load_factor_;
        std::size_t next_resize_;
    };

    /// \brief A thread-safe database adapter.
    /// \detail It derives from any database type and synchronizes access via \c std::mutex.
    template <class Database>
    class thread_safe_database : public Database
    {
    public:
        /// \brief The base database.
        typedef Database base_database;

        // workaround of lacking inheriting constructors
		template <typename ... Args>
        explicit thread_safe_database(Args&&... args)
		: base_database(std::forward<Args>(args)...) {}

        typename Database::insert_status
            insert(hash_type hash, const char *str, std::size_t length) FOONATHAN_OVERRIDE
        {
#ifdef __wasi__
#else
            std::lock_guard<std::mutex> lock(mutex_);
#endif

            return Database::insert(hash, str, length);
        }

        typename Database::insert_status
            insert_prefix(hash_type hash, hash_type prefix, const char *str, std::size_t length) FOONATHAN_OVERRIDE
        {
#ifdef __wasi__
#else
            std::lock_guard<std::mutex> lock(mutex_);
#endif
            return Database::insert_prefix(hash, prefix, str, length);
        }

        const char* lookup(hash_type hash) const FOONATHAN_NOEXCEPT FOONATHAN_OVERRIDE
        {
#ifdef __wasi__
#else
            std::lock_guard<std::mutex> lock(mutex_);
#endif
            return Database::lookup(hash);
        }

    private:
#ifdef __wasi__
#else
        mutable std::mutex mutex_;
#endif
    };

    /// \brief The default database where the strings are stored.
    /// \detail Its exact type is one of the previous listed databases.
    /// You can control its selection via the macros listed in config.hpp.in.
#if FOONATHAN_STRING_ID_DATABASE && FOONATHAN_STRING_ID_MULTITHREADED
    typedef thread_safe_database<map_database> default_database;
#elif FOONATHAN_STRING_ID_DATABASE
    typedef map_database default_database;
#else
    typedef dummy_database default_database;
#endif
}} // namespace foonathan::string_id

#endif // FOONATHAN_STRING_ID_DATABASE_HPP_INCLUDED
