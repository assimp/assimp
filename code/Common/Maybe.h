#pragma once
#include <assimp/ai_assert.h>

template <typename T>
struct Maybe {
private:
    T _val;
    bool _valid;

public:
    Maybe() :
            _valid(false) {}

    explicit Maybe(const T &val) :
            _val(val), _valid(true) {
    }

    operator bool() const {
        return _valid;
    }

    const T &Get() const {
        ai_assert(_valid);
        return _val;
    }

private:
    Maybe &operator&() = delete;
};
