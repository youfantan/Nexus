#include <cstdlib>
#include <cstring>
#include <new>
#include <thread>

#define BIT_SELECT(bits, off) (((bits) & (1 << (off))) == (1 << (off)))

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

char *HeapAllocator::reallocate(char* old_ptr, uint64_t old_size, uint64_t new_size) {
    char* ptr;
    if ((ptr = reinterpret_cast<char*>(realloc(old_ptr, new_size))) == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

/* Implements for UniquePool */

template<typename A> requires IsAllocator<A>
UniquePool<A>::UniquePool(uint64_t capacity) : capacity_(capacity) {
    memptr_ = allocator_.allocate(capacity);
}

template<typename A>requires IsAllocator<A>
UniquePool<A>::UniquePool(char *memptr, uint64_t size) : memptr_(memptr), capacity_(size) {

}

template<typename A> requires IsAllocator<A>
UniquePool<A>::UniquePool(UniquePool &&up)  noexcept : memptr_(up.memptr_), capacity_(up.capacity_), allocator_(up.allocator_) {
    up.memptr_ = nullptr;
    up.capacity_ = 0;
}

template<typename A> requires IsAllocator<A>
char &UniquePool<A>::operator[](uint64_t index) {
    return memptr_[index];
}

template<typename A> requires IsAllocator<A>
template<typename T> requires IsSimpleType<T>
Nexus::Utils::MayFail<T> UniquePool<A>::next() {
    constexpr auto step = sizeof(T);
    if (position_ + step > capacity_) {
        flag_ = flag_t::eof;
        return Nexus::Utils::failed;
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
        if (BIT_SELECT(settings_, 0)) {
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
template<typename T, size_t S> requires IsSimpleType<T>
bool UniquePool<A>::next(T (&t)[S]) {
    constexpr auto size = sizeof(T) * S;
    if (position_ + size > capacity_) {
        if (BIT_SELECT(settings_, 0)) {
            expand(size > single_automatic_expand_length ? size : single_automatic_expand_length);
        } else {
            flag_ = flag_t::eof;
            return false;
        }
    }
    memcpy(memptr_ + position_, &t[0], size);
    position_ += size;
}
/* Read the next data which type is fixed array. */
template<typename A> requires IsAllocator<A>
template<typename T, size_t S> requires IsSimpleType<T>
Nexus::Utils::MayFail<T(&)[S]> UniquePool<A>::next() {
    constexpr auto size = sizeof(T) * S;
    if (position_ + size > capacity_) {
        flag_ = flag_t::eof;
        return Nexus::Utils::failed;
    }
    T d{};
    memcpy(&d[0], memptr_ + position_, size);
    position_ += size;
    return d;
}

template<typename A> requires IsAllocator<A>
bool UniquePool<A>::write(char *ptr, uint64_t off, uint64_t len) {
    if (off + len > capacity_) {
        if (BIT_SELECT(settings_, 0)) {
            expand((off + len - capacity_) > single_automatic_expand_length ? (off + len - capacity_) : single_automatic_expand_length);
        } else {
            flag_ = flag_t::eof;
            return false;
        }
    }
    memcpy(memptr_ + off, ptr, len);
    return true;
}

template<typename A> requires IsAllocator<A>
Nexus::Utils::MayFail<UniqueFlexHolder<char>> UniquePool<A>::read(uint64_t off, uint64_t len) {
    if ((off + len) > capacity_) {
        flag_ = flag_t::eof;
        return Nexus::Utils::failed;
    }
    auto data = UniqueFlexHolder<char>(len);
    memcpy(&data.get(), memptr_ + off, len);
    return data;
}

template<typename A> requires IsAllocator<A>
void UniquePool<A>::apply_settings(const UniquePool::settings &settings) {
    settings_ = settings;
}

template<typename A> requires IsAllocator<A>
bool UniquePool<A>::expand(uint64_t expand_size) {
    expand_size += capacity_;
    if (BIT_SELECT(settings_, 0)) {
        memptr_ = allocator_.reallocate(memptr_, capacity_, expand_size);
        capacity_ = expand_size;
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
    if (BIT_SELECT(settings_, 1) && memptr_ != nullptr) {
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
uint64_t& UniquePool<A>::position() {
    return position_;
}

template<typename A> requires IsAllocator<A>
void UniquePool<A>::rewind() {
    position_ = 0;
}


/* Implement of SharedPool */
template<typename A> requires IsAllocator<A>
SharedPool<A>::SharedPool(uint64_t capacity) : allocator_(A()), mtx(new std::shared_mutex), settings_(1) {
    *capacity_ = capacity;
    memholder_ = reinterpret_cast<char**>(allocator_.allocate(8));
    *memholder_ = allocator_.allocate(capacity);
}

template<typename A>requires IsAllocator<A>
SharedPool<A>::SharedPool(char *memptr, uint64_t size) :  allocator_(A()), mtx(new std::shared_mutex) {
    *capacity_ = size;
    memholder_ = reinterpret_cast<char**>(allocator_.allocate(8));
    *memholder_ =  memptr;
}

template<typename A> requires IsAllocator<A>
SharedPool<A>::SharedPool(const SharedPool &up) : allocator_(up.allocator_), capacity_(up.capacity_), memholder_(up.memholder_), mtx(up.mtx), settings_((up.settings_)) {
    if (!adjust_refcount(1)) {
        throw std::runtime_error("Coping SharedPool when being destructed.");
    }
}

template<typename A> requires IsAllocator<A>
char &SharedPool<A>::operator[](uint64_t index) {
    return *memholder_[index];
}

template<typename A> requires IsAllocator<A>
template<typename T> requires IsSimpleType<T>
Nexus::Utils::MayFail<T> SharedPool<A>::next() {
    constexpr auto step = sizeof(T);
    mtx->lock_shared();
    if (position_ + step > *capacity_) {
        flag_ = flag_t::eof;
        mtx->unlock_shared();
        return Nexus::Utils::failed;
    }
    T d{};
    memcpy(&d, *memholder_ + position_, step);
    position_ += step;
    mtx->unlock_shared();
    return d;
}

template<typename A> requires IsAllocator<A>
template<typename T> requires IsSimpleType<T>
bool SharedPool<A>::next(T t) {
    constexpr auto step = sizeof(T);
    mtx->lock();
    if (position_ + step > *capacity_) {
        if (BIT_SELECT(settings_, 0)) {
            expand(step > single_automatic_expand_length ? position_ + step : position_ + single_automatic_expand_length);
        } else {
            flag_ = flag_t::eof;
            mtx->unlock();
            return false;
        }
    }
    memcpy(*memholder_ + position_, &t, step);
    position_ += step;
    mtx->unlock();
    return true;
}

template<typename A> requires IsAllocator<A>
template<typename T, size_t S> requires IsSimpleType<T>
bool SharedPool<A>::next(T (&t)[S]) {
    constexpr auto size = sizeof(T) * S;
    mtx->lock();
    if (position_ + size > *capacity_) {
        if (BIT_SELECT(settings_, 0)) {
            expand(size > single_automatic_expand_length ? position_ + size : position_ + single_automatic_expand_length);
        } else {
            flag_ = flag_t::eof;
            mtx->unlock();
            return false;
        }
    }
    memcpy(*memholder_ + position_, &t[0], size);
    position_ += size;
    mtx->unlock();
    return true;
}

template<typename A> requires IsAllocator<A>
template<typename T, size_t S> requires IsSimpleType<T>
Nexus::Utils::MayFail<T[S]> SharedPool<A>::next() {
    constexpr auto size = sizeof(T) * S;
    mtx->lock_shared();
    if (position_ + size > *capacity_) {
        flag_ = flag_t::eof;
        mtx->unlock_shared();
        return Nexus::Utils::failed;
    }
    T d{};
    memcpy(&d[0], *memholder_ + position_, size);
    position_ += size;
    mtx->unlock_shared();
    return d;
}

template<typename A> requires IsAllocator<A>
bool SharedPool<A>::write(char *ptr, uint64_t off, uint64_t len) {
    mtx->lock();
    if (off + len > *capacity_) {
        if (BIT_SELECT(settings_, 0)) {
            expand((off + len - *capacity_) > single_automatic_expand_length ? (off + len - *capacity_) : single_automatic_expand_length);
        } else {
            flag_ = flag_t::eof;
            return false;
        }
    }
    memcpy(*memholder_ + off, ptr, len);
    return true;
}

template<typename A> requires IsAllocator<A>Nexus::Utils::MayFail<UniqueFlexHolder<char>>
SharedPool<A>::read(uint64_t off, uint64_t len) {
    mtx->lock_shared();
    if ((off + len) > *capacity_) {
        flag_ = flag_t::eof;
        return Nexus::Utils::failed;
    }
    auto data = UniqueFlexHolder<char>(len);
    memcpy(&data.get(), *memholder_ + off, len);
    mtx->unlock_shared();
    return data;
}

template<typename A> requires IsAllocator<A>
void SharedPool<A>::apply_settings(const SharedPool::settings &settings) {
    settings_ = settings;
}

template<typename A> requires IsAllocator<A>
bool SharedPool<A>::expand(uint64_t new_capacity) {
    if (BIT_SELECT(settings_, 0)) {
        *memholder_ = allocator_.reallocate(*memholder_, *capacity_, new_capacity);
        *capacity_ = new_capacity;
        return true;
    }
    return false;
}

template<typename A> requires IsAllocator<A>
SharedPool<A>::~SharedPool() {
    release();
}

template<typename A> requires IsAllocator<A>
void SharedPool<A>::release() {
    if (memholder_ != nullptr) {
        if (!adjust_refcount(-1)) {
            allocator_.recycle(*memholder_, *capacity_);
            delete memholder_;
            memholder_ = nullptr;
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // wait 10ms to make sure all the conflicting threads could pass.
            delete mtx;
            delete reference_counting;
            delete capacity_;
        }
        memholder_ = nullptr;
    }
}

template<typename A> requires IsAllocator<A>
void SharedPool<A>::close() {
    release();
}

template<typename A> requires IsAllocator<A>
SharedPool<A>::flag_t SharedPool<A>::flag() {
    SharedPool::flag_t result;
    return result;
}

template<typename A> requires IsAllocator<A>
void SharedPool<A>::position(uint64_t npos) {
    position_ = npos;
}

template<typename A> requires IsAllocator<A>
uint64_t& SharedPool<A>::position() {
    return position_;
}

template<typename A> requires IsAllocator<A>
void SharedPool<A>::rewind() {
    position_ = 0;
}

template<typename A> requires IsAllocator<A>
bool SharedPool<A>::adjust_refcount(int refcount) {
    mtx->lock();
    if (*reference_counting <= 0) {
        return false;
    }
    *reference_counting += refcount;
    if (*reference_counting == 0) {
        return false;
    }
    mtx->unlock();
    return true;
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
Nexus::Utils::MayFail<T> Stream<C>::next() {
    return container_.template next<T>();
}

template<typename C> requires ContainerStreamable<C>
template<typename T> requires IsSimpleType<T>
bool Stream<C>::next(T t) {
    return container_.next(t);
}

template<typename C> requires ContainerStreamable<C>
template<typename T, size_t S> requires IsSimpleType<T>
bool Stream<C>::next(T (&t)[S]) {
    return container_.next(t);
}
template<typename C> requires ContainerStreamable<C>
template<typename T, size_t S> requires IsSimpleType<T>
Nexus::Utils::MayFail<T[S]> Stream<C>::next() {
    return container_.template next<T[S]>();
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
uint64_t& Stream<C>::position() {
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

template<typename C>
requires ContainerStreamable<C>C &Stream<C>::container() {
    return container_;
}

template<typename C>
requires ContainerStreamable<C>
bool Stream<C>::write(char *ptr, uint64_t len) {
    if (!container_.write(ptr, position(), len)) return false;
    position() += len;
    return true;
}

template<typename C>
requires ContainerStreamable<C>
Nexus::Utils::MayFail<UniqueFlexHolder<char>> Stream<C>::read(uint64_t len) {
    Nexus::Utils::MayFail<UniqueFlexHolder<char>> r = container_.read(position(), len);
    if (r.is_valid()) position() += len;
    return r;
}

template<typename T, typename A> requires IsAllocator<A>
UniqueFlexHolder<T, A>::UniqueFlexHolder() : size_(0), buffer_(nullptr) {}

template<typename T, typename A> requires IsAllocator<A>
UniqueFlexHolder<T, A>::UniqueFlexHolder(const T &t) : size_(sizeof(T)) {
    buffer_ = allocator_.allocate(sizeof(T));
    memcpy(buffer_, &t, sizeof(T));
}

template<typename T, typename A> requires IsAllocator<A>
UniqueFlexHolder<T, A>::UniqueFlexHolder(uint64_t capacity) : size_(capacity) {
    buffer_ = allocator_.allocate(capacity);
}

template<typename T, typename A> requires IsAllocator<A>
bool UniqueFlexHolder<T, A>::assign(void *anyptr) {
    if (memcpy(buffer_, anyptr, size_) == nullptr) {
        return false;
    }
    return true;
}

template<typename T, typename A> requires IsAllocator<A>
bool UniqueFlexHolder<T, A>::assign(const T &t) {
    if (memcpy(buffer_, reinterpret_cast<char*>(&t), size_) == nullptr) {
        return false;
    }
    return true;
}

template<typename T, typename A> requires IsAllocator<A>
UniqueFlexHolder<T, A>::~UniqueFlexHolder() {
    if (buffer_ != nullptr) {
        allocator_.recycle(buffer_, size_);
        buffer_ = nullptr;
    }
}

template<typename T, typename A> requires IsAllocator<A>
T* UniqueFlexHolder<T, A>::ptr() {
    return reinterpret_cast<T*>(buffer_);
}

template<typename T, typename A> requires IsAllocator<A>
T&UniqueFlexHolder<T, A>::get() {
    return *reinterpret_cast<T*>(buffer_);
}

template<typename T, typename A> requires IsAllocator<A>
UniqueFlexHolder<T, A>::UniqueFlexHolder(UniqueFlexHolder &&holder)  noexcept : buffer_(holder.buffer_), size_(holder.size_), allocator_(holder.allocator_) {
    holder.buffer_ = nullptr;
}
