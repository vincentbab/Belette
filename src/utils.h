#ifndef TIMEMANAGER_H_INCLUDED
#define TIMEMANAGER_H_INCLUDED

#include <chrono>
#include <string>
#include <cstddef>
#include <cstdint>
#include <new>
#if defined(__linux__)
#include <sys/mman.h>
#endif

namespace Belette {

using TimeMs = std::chrono::milliseconds::rep;

inline TimeMs now() {
    return std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now().time_since_epoch()).count();
}

#if defined(__linux__)
constexpr size_t LARGE_ALLOC_ALIGNMENT = 2 * 1024 * 1024;
#else
constexpr size_t LARGE_ALLOC_ALIGNMENT = 4096;
#endif

inline void *largeAlloc(size_t size) {
    size = (size + LARGE_ALLOC_ALIGNMENT - 1) / LARGE_ALLOC_ALIGNMENT * LARGE_ALLOC_ALIGNMENT;
    void *mem = ::operator new(size, std::align_val_t(LARGE_ALLOC_ALIGNMENT), std::nothrow);
#if defined(__linux__) && defined(MADV_HUGEPAGE)
    if (mem) madvise(mem, size, MADV_HUGEPAGE);
#endif
    return mem;
}

inline void largeFree(void *mem) {
    ::operator delete(mem, std::align_val_t(LARGE_ALLOC_ALIGNMENT));
}

inline int parseInt(const std::string &str) {
    try {
        return std::stoi(str);
    } catch (const std::invalid_argument & e) {
        return 0;
    } catch (const std::out_of_range & e) {
        return 0;
    }

    return 0;
}

inline int64_t parseInt64(const std::string &str) {
    try {
        return std::stoll(str);
    } catch (const std::invalid_argument & e) {
        return 0;
    } catch (const std::out_of_range & e) {
        return 0;
    }

    return 0;
}

class FastRandGenerator {
public:
    explicit FastRandGenerator(uint64_t seed = 1234567890) : state(seed) { }

    inline uint64_t rand() {
        uint64_t z = (state += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }

    inline uint64_t rand(uint64_t n) { return rand() % n; }

private:
    uint64_t state;
};

// https://stackoverflow.com/questions/9779105/generic-member-function-pointer-as-a-template-parameter
template <typename T, typename R, typename ...Args>
class MemberFunctionProxy {
public:
    typedef void (T::*Func)(Args...);
    
    inline MemberFunctionProxy() { };
    inline MemberFunctionProxy(Func func_): func(func_) { };
    inline R operator()(T &obj, Args ...args) { (obj.*func)(std::forward<Args>(args)...); }
private:
    Func func;
};

} /* namespace Belette */

#endif /* TIMEMANAGER_H_INCLUDED */
