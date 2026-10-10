/*
 * GEOHelper.h
 *
 *  Created on: 17.09.2014
 *      Author: zsolt
 */

#pragma once
#ifndef GEOHELPER_H_
#define GEOHELPER_H_

#include <cerrno>
#include <climits>
#include <cstdlib>

// Parse a hex color token; advances *out past the consumed digits when provided.
inline unsigned int hexstrtoul10(const char *in, const char **out = nullptr) {
    char *end = nullptr;
    errno = 0;
    const unsigned long long result = std::strtoull(in, &end, 16);
    if (out != nullptr) {
        *out = end != nullptr ? end : in;
    }
    if (result == 0 && end == in) {
        return 0;
    }
    if (result == ULLONG_MAX && errno != 0) {
        return 0;
    }
    return static_cast<unsigned int>(result);
}

#endif /* GEOHELPER_H_ */
