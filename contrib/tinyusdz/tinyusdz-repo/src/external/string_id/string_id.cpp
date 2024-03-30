// Copyright (C) 2014-2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "string_id.hpp"

#include "error.hpp"

namespace sid = foonathan::string_id;

namespace
{
    void handle_collision(sid::basic_database &db, sid::hash_type hash, const char *str)
    {
        auto handler = sid::get_collision_handler();
        auto second = db.lookup(hash);
        handler(hash, str, second);
    }
}

sid::string_id::string_id(string_info str, basic_database &db)
{
    basic_database::insert_status status;
    *this = string_id(str, db, status);
    if (!status)
        handle_collision(*db_, id_, str.string);
}

sid::string_id::string_id(string_info str, basic_database &db,
                          basic_database::insert_status &status)
: id_(detail::sid_hash(str.string)), db_(&db)
{
    status = db_->insert(id_, str.string, str.length);
}

sid::string_id::string_id(const string_id &prefix, string_info str)
{
    basic_database::insert_status status;
    *this = string_id(prefix, str, status);
    if (!status)
        handle_collision(*db_, id_, str.string);
}

sid::string_id::string_id(const string_id &prefix, string_info str,
                          basic_database::insert_status &status)
: id_(detail::sid_hash(str.string, prefix.hash_code())), db_(prefix.db_)
{
    status = db_->insert_prefix(id_, prefix.hash_code(), str.string, str.length);
}

const char* sid::string_id::string() const FOONATHAN_NOEXCEPT
{
    return db_->lookup(id_);
}

