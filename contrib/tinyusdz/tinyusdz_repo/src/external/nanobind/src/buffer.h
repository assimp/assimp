#include <string.h>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

struct Buffer {
public:
    // Disable copy/move constructor and assignment
    Buffer(const Buffer &) = delete;
    Buffer(Buffer &&) = delete;
    Buffer &operator=(const Buffer &) = delete;
    Buffer &operator=(Buffer &&) = delete;

    Buffer(size_t size = 0)
        : m_start((char *) malloc(size)) {
        if (!m_start) {
            fprintf(stderr, "Buffer::Buffer(): out of memory (unrecoverable error)!");
            abort();
        }
        m_end = m_start + size;
        if (size)
            clear();
    }

    ~Buffer() {
        free(m_start);
    }

    /// Append a string with the specified length
    void put(const char *str, size_t size) {
        if (m_cur + size >= m_end)
            expand(size + 1 - remain());

        memcpy(m_cur, str, size);
        m_cur += size;
        *m_cur = '\0';
    }

    /// Append a string
    template <size_t N> void put(const char (&str)[N]) {
        put(str, N - 1);
    }

    /// Append a dynamic string
    void put_dstr(const char *str) { put(str, strlen(str)); }

    /// Append a single character to the buffer
    void put(char c) {
        if (m_cur + 1 >= m_end)
            expand();
        *m_cur++ = c;
        *m_cur = '\0';
    }

    /// Append multiple copies of a single character to the buffer
    void put(char c, size_t count) {
        if (m_cur + count >= m_end)
            expand(count + 1 - remain());
        for (size_t i = 0; i < count; ++i)
            *m_cur++ = c;
        *m_cur = '\0';
    }

    /// Append a formatted (printf-style) string to the buffer
#if defined(__GNUC__)
    __attribute__((__format__ (__printf__, 2, 3)))
#endif
    size_t fmt(const char *format, ...) {
        size_t written;
        do {
            size_t size = remain();
            va_list args;
            va_start(args, format);
            written = (size_t) vsnprintf(m_cur, size, format, args);
            va_end(args);

            if (written + 1 < size) {
                m_cur += written;
                break;
            }

            expand();
        } while (true);

        return written;
    }

    const char *get() { return m_start; }

    void clear() {
        m_cur = m_start;
        if (m_start != m_end)
            m_start[0] = '\0';
    }

    /// Remove the last 'n' characters
    void rewind(size_t n) {
        if (m_cur < m_start + n)
            m_cur = m_start;
        else
            m_cur -= n;
        *m_cur = '\0';
    }

    /// Append an unsigned 32 bit integer
    void put_uint32(uint32_t value) {
        const int digits = 10;
        const char *num = "0123456789";
        char buf[digits];
        int i = digits;

        do {
            buf[--i] = num[value % 10];
            value /= 10;
        } while (value);

        return put(buf + i, digits - i);
    }

    char *copy(size_t offset = 0) const {
        size_t copy_size = size() + 1 - offset;
        char *tmp = (char *) malloc(copy_size);
        if (!tmp) {
            fprintf(stderr, "Buffer::copy(): out of memory (unrecoverable error)!");
            abort();
        }
        memcpy(tmp, m_start + offset, copy_size);
        return tmp;
    }

    size_t size() const { return m_cur - m_start; }
    size_t remain() const { return m_end - m_cur; }

private:
    void expand(size_t minval = 2) {
        size_t old_alloc_size = m_end - m_start,
               new_alloc_size = 2 * old_alloc_size + minval,
               used_size      = m_cur - m_start,
               copy_size      = used_size + 1;

        if (old_alloc_size < copy_size)
            copy_size = old_alloc_size;

        char *tmp = (char *) malloc(new_alloc_size);
        if (!tmp) {
            fprintf(stderr, "Buffer::expand(): out of memory (unrecoverable error)!");
            abort();
        }

        memcpy(tmp, m_start, copy_size);
        free(m_start);

        m_start = tmp;
        m_end = m_start + new_alloc_size;
        m_cur = m_start + used_size;
    }

private:
    char *m_start{nullptr}, *m_cur{nullptr}, *m_end{nullptr};
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
