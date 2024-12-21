#include <cstdlib>
#include <cstring>
#include <new>

using namespace Nexus::Base;

char* HeapAllocator::allocate(uint64_t size) {
    char* ptr;
    if ((ptr = reinterpret_cast<char*>(malloc(size))) == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

bool HeapAllocator::recycle(char *ptr, uint64_t size) {
    free(ptr);
    return true;
}

char *HeapAllocator::reallocate(char *old_ptr, uint64_t old_size, uint64_t new_size) {
    char* ptr;
    if ((ptr = reinterpret_cast<char*>(realloc(old_ptr, new_size))) == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

/* Implements for UniquePool */

template<typename A> requires IsAllocator<A>
UniquePool<A>::UniquePool(uint64_t capacity) : allocator_(A()), memptr_(allocator_.allocate(capacity)), capacity_(capacity) {

}

template<typename A>requires IsAllocator<A>
UniquePool<A>::UniquePool(char *memptr, uint64_t size) :  allocator_(A()), memptr_(memptr), capacity_(size) {

}

template<typename A> requires IsAllocator<A>
UniquePool<A>::UniquePool(UniquePool &&up) : memptr_(up.memptr_), capacity_(up.capacity_), allocator_(up.allocator_) {
    up.memptr_ = nullptr;
    up.capacity_ = 0;
}

template<typename A> requires IsAllocator<A>
template<typename T> requires IsSimpleType<T>
Nexus::Check::mayfail<T> UniquePool<A>::next() {
    constexpr auto step = sizeof(T);
    if (position_ + step > capacity_) {
        flag_ = flag_t::eof;
        return Nexus::Check::failed;
    }
    T d{};
    memcpy(&d, memptr_ + position_, step);
    position_ += step;
    return d;
}

template<typename A> requires IsAllocator<A>
template<typename T> requires IsSimpleType<T>
bool UniquePool<A>::next(T t) {
    constexpr auto step = sizeof(T);
    if (position_ + step > capacity_) {
        if (settings_.auto_expand) {
            expand(step > single_automatic_expand_length ? step : single_automatic_expand_length);
        } else {
            flag_ = flag_t::eof;
            return false;
        }
    }
    memcpy(memptr_ + position_, &t, step);
    position_ += step;
    return true;
}

template<typename A> requires IsAllocator<A>
void UniquePool<A>::apply_settings(const UniquePool::settings &settings) {
    settings_ = settings;
}

template<typename A> requires IsAllocator<A>
bool UniquePool<A>::expand(uint64_t new_capacity) {
    if (settings_.auto_expand) {
        memptr_ = allocator_.reallocate(memptr_, capacity_, new_capacity);
        return true;
    }
    return false;
}

template<typename A> requires IsAllocator<A>
UniquePool<A>::~UniquePool() {
    release();
}

template<typename A> requires IsAllocator<A>
void UniquePool<A>::release() {
    if (settings_.auto_free && memptr_ != nullptr) {
        allocator_.recycle(memptr_, capacity_);
        memptr_ = nullptr;
    }
}

template<typename A> requires IsAllocator<A>
void UniquePool<A>::close() {
    release();
}

template<typename A> requires IsAllocator<A>
UniquePool<A>::flag_t UniquePool<A>::flag() {
    UniquePool::flag_t result;
    return result;
}

template<typename A> requires IsAllocator<A>
void UniquePool<A>::position(uint64_t npos) {
    position_ = npos;
}

template<typename A> requires IsAllocator<A>
uint64_t UniquePool<A>::position() {
    return position_;
}

template<typename A> requires IsAllocator<A>
void UniquePool<A>::rewind() {
    position_ = 0;
}



/* Implements for Stream */

template<typename C> requires ContainerStreamable<C>
Stream<C>::Stream(const C &container) : container_(container) {

}

template<typename C> requires ContainerStreamable<C>
Stream<C>::Stream(C &&container) : container_(std::move(container)) {

}

template<typename C> requires ContainerStreamable<C>
template<typename T> requires IsSimpleType<T>
Nexus::Check::mayfail<T> Stream<C>::next() {
    return container_.template next<T>();
}

template<typename C> requires ContainerStreamable<C>
template<typename T> requires IsSimpleType<T>
bool Stream<C>::next(T t) {
    return container_.next(t);
}

template<typename C> requires ContainerStreamable<C>
void Stream<C>::close() {
    container_.close();
}

template<typename C> requires ContainerStreamable<C>
C::flag_t Stream<C>::flag() {
    return container_.flag();
}

template<typename C> requires ContainerStreamable<C>
void Stream<C>::position(uint64_t npos) {
    container_.position(npos);
}

template<typename C> requires ContainerStreamable<C>
uint64_t Stream<C>::position() {
    return container_.position();
}

template<typename C> requires ContainerStreamable<C>
void Stream<C>::rewind() {
    container_.rewind();
}

template<typename C> requires ContainerStreamable<C>
Stream<C>::~Stream() {
    close();
}