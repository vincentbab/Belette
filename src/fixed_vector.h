#ifndef FIXED_VECTOR_H_INCLUDED
#define FIXED_VECTOR_H_INCLUDED

#include <initializer_list>
#include <cassert>
#include <cstdint>
#include <algorithm>

namespace Belette {

template <typename T, int N, typename SizeT = uint32_t> class fixed_vector {
    T elements[N];
    SizeT count;
public:
    typedef T* iterator;
    typedef const T* const_iterator;

    inline iterator begin() { return &elements[0]; }
    inline const_iterator begin() const { return &elements[0]; }
    inline iterator end() { return &elements[count]; }
    inline const_iterator end() const { return &elements[count]; }

    fixed_vector() noexcept : count(0) {}
    inline fixed_vector(std::initializer_list<T> il) {copy(il.begin(), il.end(), begin());count = il.end() - il.begin();}
    inline fixed_vector(fixed_vector const &ml) = default;
    inline fixed_vector(fixed_vector&&)=default;
    inline fixed_vector& operator=(fixed_vector const &ml)=default;
    inline fixed_vector& operator=(fixed_vector&&)=default;
    inline T &operator[](SizeT i) {assert(i < count); return elements[i];}
    inline const T &operator[](SizeT i) const {assert(i < count); return elements[i];}
    inline const T &front() const {assert(count > 0); return elements[0]; }
    inline T &front() {assert(count > 0); return elements[0]; }
    inline const T &back() const {assert(count > 0); return elements[count-1]; }
    inline T &back() {assert(count > 0); return elements[count-1]; }
    inline void push_back(const T& e) {assert(count < N); elements[count++] = e;}
    inline void push_back(T&& e) {assert(count < N); elements[count++] = e;}
    inline void pop_back() {assert(count > 0); count--;}
    inline void clear() {count = 0;}
    inline SizeT size() const {return count;}
    inline void resize(SizeT s) {assert(s <= count); count = s;} // only resize to smaller count
    inline bool empty() const {return count==0;}
    inline bool contains(const T &e) { return std::find(begin(), end(), e) != end(); }
    inline SizeT capacity() const {return N;}
    inline iterator erase (const_iterator _pos) {
        iterator pos = begin() + (_pos - begin());
        std::copy(pos+1, end(), pos);count--;
        return pos;
    }
    inline iterator erase (const_iterator _first, const_iterator _last) {
        iterator first = begin() + (_first - begin());
        iterator last = begin() + (_last - begin());
        std::copy(last, end(), first);
        this->count -= last-first;return first;
    }
    
    inline iterator insert(const_iterator _first, const_iterator _last) {
        assert(std::distance(_first, _last) + count <= N);
        std::copy(_first, _last, end());
        count += std::distance(_first, _last);
        return end();
    }

    template<typename Compare>
    inline void insert_sorted(const T& e, Compare comp) {
        T *cur = end();

        for (; cur != begin() && comp(e, *(cur - 1)); cur--) {
            *cur = *(cur - 1);
        }

        *cur = e;
        count++;
    }

    template<typename... Args> inline void emplace_back(Args&&... args) {
        assert(count < N);
        new(&elements[count++]) T(std::forward<Args>(args)...);
    }
};

} /* namespace Belette */

#endif /* FIXED_VECTOR_H_INCLUDED */
