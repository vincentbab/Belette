#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <initializer_list>

namespace BabChess {

template <typename T, int N, typename SizeT = uint32_t> class Array {
    T elements[N];
    SizeT count;
public:
    typedef T* iterator;
    typedef const T* const_iterator;

    inline iterator begin() { return &elements[0]; }
    inline const_iterator begin() const { return &elements[0]; }
    inline iterator end() { return &elements[count]; }
    inline const_iterator end() const { return &elements[count]; }

    Array() : count(0) {}
    inline Array(std::initializer_list<T> il) {copy(il.begin(), il.end(), begin());count = il.end() - il.begin();}
    inline Array(Array const &ml) = default;
    inline Array(Array&&)=default;
    inline Array& operator=(Array const &ml)=default;
    inline Array& operator=(Array&&)=default;
    inline T &operator[](SizeT i) {assert(i < count); return elements[i];}
    inline const T &operator[](SizeT i) const {assert(i < count); return elements[i];}
    inline const T &front() const {assert(count > 0); return elements[0]; }
    inline T &front() {assert(count > 0); return elements[0]; }
    inline const T &back() const {assert(count > 0); return elements[count-1]; }
    inline T &back() {assert(count > 0); return elements[count-1]; }
    inline void push_back(T e) {assert(count < N); elements[count++] = e;}
    inline void pop_back() {assert(count > 0); count--;}
    inline void clear() {count = 0;}
    inline SizeT size() const {return count;}
    inline void resize(SizeT s) {assert(s <= count); count = s;} // only resize to smaller count
    inline bool empty() const {return count==0;}
    inline SizeT capacity() const {return N;}
    inline iterator erase (const_iterator _pos) {iterator pos = begin() + (_pos - begin());copy(pos+1, end(), pos);count--;return pos;}
    inline iterator erase (const_iterator _first, const_iterator _last) {iterator first = begin() + (_first - begin());iterator last = begin() + (_last - begin());copy(last, end(), first);this->count -= last-first;return first;}
};

} /* namespace BabChess */

#endif /* ARRAY_H_INCLUDED */
