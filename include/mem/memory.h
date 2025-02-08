#pragma once

#include <cstdint>
#include <concepts>
#include <shared_mutex>
#include <cstdlib>
#include <cstring>
#include <new>
#include <thread>
#include "../utils/check.h"

#define BIT_SELECT(bits, off) (((bits) & (1 << (off))) == (1 << (off)))

namespace Nexus::Base {
    template<typename A>
    concept IsAllocator = requires(A a) {
        { a.allocate(UINT64_MAX) } -> std::convertible_to<char*>;
        { a.reallocate(nullptr, UINT64_MAX, UINT64_MAX) } -> std::convertible_to<char*>;
        { a.recycle(nullptr, UINT64_MAX) } -> std::same_as<bool>;
    };

    class HeapAllocator {
    public:
        char* allocate(uint64_t size) {
            char* ptr;
            if ((ptr = reinterpret_cast<char*>(calloc(1, size))) == nullptr) {
                throw std::bad_alloc();
            }
            return ptr;
        }

        char* reallocate(char* old_ptr, uint64_t old_size, uint64_t new_size) {
            char* ptr;
            if ((ptr = reinterpret_cast<char*>(realloc(old_ptr, new_size))) == nullptr) {
                throw std::bad_alloc();
            }
            return ptr;
        }

        bool recycle(char* ptr, uint64_t size) {
            free(ptr);
            return true;
        }
    };

    template<typename T, typename A = HeapAllocator> requires IsAllocator<A>
    class UniqueFlexHolder;

    struct plain_type_for_constraint {
        uint32_t a;
        char b[14];
        float c;
        bool d;
    };
    static char fixed_array_type_for_constraint[4] = {1, 2, 3, 4};

    template<typename C>
    concept ContainerStreamable = requires(C c) {
        { c.template next<plain_type_for_constraint>() } -> std::same_as<Nexus::Utils::MayFail<plain_type_for_constraint>>;
        { c.template next<plain_type_for_constraint>(plain_type_for_constraint()) } -> std::same_as<bool>;
        { c.template next<decltype(fixed_array_type_for_constraint)>() } -> std::same_as<Nexus::Utils::MayFail<decltype(fixed_array_type_for_constraint)>>;
        { c.template next<decltype(fixed_array_type_for_constraint)>(fixed_array_type_for_constraint) } -> std::same_as<bool>;
        { c.read(UINT64_MAX) } -> std::same_as<Nexus::Utils::MayFail<UniqueFlexHolder<char>>>;
        { c.write(nullptr, UINT64_MAX) } -> std::same_as<bool>;
        { c.rewind() };
        { c.position() } -> std::same_as<uint64_t&>;
        { c.limit() } -> std::same_as<uint64_t>;
        { c.position(UINT64_MAX) };
        { c.flag() } -> std::same_as<typename C::flag_t>;
        { c.close() };
    };

    template<typename T>
    concept IsSimpleType = std::is_fundamental_v<T> || (std::is_class_v<T> && !std::is_member_function_pointer_v<T>) || std::is_array_v<T>;

    /*
     * UniqueFlexHolder provides a simple instead of unique_ptr, ensured both secure and flexibility.
     * */
    template<typename T, typename A> requires IsAllocator<A>
    class UniqueFlexHolder {
        friend Nexus::Utils::MayFail<UniqueFlexHolder>;
    private:
        A allocator_;
        uint64_t size_;
        char* buffer_;
    public:
        UniqueFlexHolder() : size_(0), buffer_(nullptr) {}
        explicit UniqueFlexHolder(const T& t) : size_(sizeof(T)) {
            buffer_ = allocator_.allocate(sizeof(T));
            memcpy(buffer_, &t, sizeof(T));
        }

        explicit UniqueFlexHolder(uint64_t capacity) : size_(capacity) {
            buffer_ = allocator_.allocate(capacity);
        }

        UniqueFlexHolder(UniqueFlexHolder&) = delete;
        UniqueFlexHolder(UniqueFlexHolder&& holder) noexcept : buffer_(holder.buffer_), size_(holder.size_), allocator_(holder.allocator_) {
            holder.buffer_ = nullptr;
        }

        char& operator[](uint64_t index) {
#ifdef DEBUG
            if(index > size_) {
                std::runtime_error("UniqueFlexHolder Out of Bound");
            }
#endif
            return buffer_[index];
        }

        bool assign(void* anyptr) {
            if (memcpy(buffer_, anyptr, size_) == nullptr) {
                return false;
            }
            return true;
        }

        bool assign(const T& t) {
            if (memcpy(buffer_, reinterpret_cast<char*>(&t), size_) == nullptr) {
                return false;
            }
            return true;
        }

        ~UniqueFlexHolder() {
            if (buffer_ != nullptr) {
                allocator_.recycle(buffer_, size_);
                buffer_ = nullptr;
            }
        }

        T* ptr() {
            return reinterpret_cast<T*>(buffer_);
        }

        T& get() {
            return *reinterpret_cast<T*>(buffer_);
        }

        uint64_t size() {
            return size_;
        }
    };

    template<typename C> requires ContainerStreamable<C>
    class Stream {
    private:
        C container_;
    public:
        /* Construct stream with a reference of container. */
        explicit Stream(const C& container) : container_(container) {}
        /* Construct stream with a right reference of container. */
        explicit Stream(C&& container) : container_(std::move(container)) {}
        /* Read the next data. */
        template<typename T> requires IsSimpleType<T>
        Nexus::Utils::MayFail<T> next() {
            return container_.template next<T>();
        }
        /* Write the next data. */
        template<typename T> requires IsSimpleType<T>
        bool next(T t) {
            return container_.next(t);
        }
        /* Read the next data which type is fixed array. */
        template<typename T, size_t S> requires IsSimpleType<T>
        Nexus::Utils::MayFail<T[S]> next() {
            return container_.template next<T[S]>();
        }
        /* Write the next data which type is fixed array. */
        template<typename T, size_t S> requires IsSimpleType<T>
        bool next(T(&t)[S]) {
            return container_.next(t);
        }
        /* Read data in specified size. */
        Nexus::Utils::MayFail<UniqueFlexHolder<char>> read(uint64_t len) {
            return container_.read(len);
        }
        /* Write data in specified size. */
        bool write(char* ptr, uint64_t len) {
            return container_.write(ptr, len);

        }
        /* Rewind the container. */
        void rewind() {
            container_.rewind();
        }
        /* Get the position of container. */
        uint64_t& position() {
            return container_.position();
        }
        /* Set the position of container. */
        void position(uint64_t npos) {
            container_.position(npos);
        }
        /* Get the limit of container */
        uint64_t limit() {
            return container_.limit();
        }
        /* Get the last operation flag of container. */
        C::flag_t flag() {
            return container_.flag();
        }
        /* Get the reference of the container. */
        C& container() {
            return container_;
        }
        /* Call this function only when you need to release the container before Stream destruction automatically. When Stream is being
         * destructed, it will close the container.  */
        void close() {
            container_.close();
        }
        ~Stream() {
            close();
        }
    };

    /*
     * UniquePool provides a safe container to manage memory, which ensures that only one running thread can hold it at any time.
     * To ensure flexibility, this class provides an optional template parameter A for using different memory allocation methods.
     * */
    template<typename A = HeapAllocator> requires IsAllocator<A>
    class UniquePool {
    public:
        static constexpr uint64_t single_automatic_expand_length = 1024;
        using settings = char;
        enum class flag_t {
            eof,
            normal
        };
    private:
        char* memptr_;
        uint64_t capacity_;
        uint64_t position_{0};
        uint64_t limit_{0};
        A allocator_{};
        // Bit Assign: [7:2] Reserved [2:1] Auto Free [1:0] Auto Expand
        settings settings_{0b00000011};
        flag_t flag_ {flag_t::normal};
    public:
        /* Allocate a new UniquePool with given capacity. */
        explicit UniquePool(uint64_t capacity)  : capacity_(capacity) {
            memptr_ = allocator_.allocate(capacity);
        }
        /* Use UniquePool to manage a pointer and carefully confirm the life cycle of the pointer. */
        UniquePool(char* memptr, uint64_t size) : memptr_(memptr), capacity_(size) {

        }
        /* UniquePool cannot be copied. */
        UniquePool(const UniquePool& up) = delete;
        /* UniquePool can be moved. */
        UniquePool(UniquePool&& up) noexcept : memptr_(up.memptr_), capacity_(up.capacity_), allocator_(up.allocator_), limit_(up.limit_) {
            up.memptr_ = nullptr;
            up.capacity_ = 0;
        }
        /* Direct access to the buffer. */
        char& operator[](uint64_t index) {
            return memptr_[index];
        }
        /* Read data in specified size with specified position. */
        Nexus::Utils::MayFail<UniqueFlexHolder<char>> read(uint64_t off, uint64_t len) {
            flag_ = flag_t::normal;
            if (position_ >= limit_) {
                flag_ = flag_t::eof;
                return Nexus::Utils::failed;
            }
            if (position_ + len >= limit_) {
                len = limit_ - position_;
            }
            auto data = UniqueFlexHolder<char>(len);
            memcpy(&data.get(), memptr_ + position_, len);
            position_ += len;
            return data;
        }

        /* Read data in specified size with position. */
        Nexus::Utils::MayFail<UniqueFlexHolder<char>> read(uint64_t len) {
            flag_ = flag_t::normal;
            if ((position_ + len) > limit_) {
                flag_ = flag_t::eof;
                return Nexus::Utils::failed;
            }
            auto data = UniqueFlexHolder<char>(len);
            memcpy(&data.get(), memptr_ + position_, len);
            position_ += len;
            return data;
        }

        /* Write data in specified size with specified position. */
        bool write(char* ptr, uint64_t off, uint64_t len) {
            if (off + len > capacity_) {
                if (BIT_SELECT(settings_, 0)) {
                    expand((off + len - capacity_) > single_automatic_expand_length ? (off + len - capacity_) : single_automatic_expand_length);
                } else {
                    return false;
                }
            }
            memcpy(memptr_ + off, ptr, len);
            return true;
        }

        /* Write data in specified size with position. */
        bool write(char* ptr, uint64_t len) {
            flag_ = flag_t::normal;
            if (position_ + len > capacity_) {
                if (BIT_SELECT(settings_, 0)) {
                    expand((position_ + len - capacity_) > single_automatic_expand_length ? (position_ + len - capacity_) : single_automatic_expand_length);
                } else {
                    flag_ = flag_t::eof;
                    return false;
                }
            }
            if (position_ + len == capacity_) flag_ = flag_t::eof;
            memcpy(memptr_ + position_, ptr, len);
            limit_ += len;
            position_ += len;
            return true;
        }

        /* Apply settings for UniquePool */
        void apply_settings(const settings& settings)  {
            settings_ = settings;
        }
        bool expand(uint64_t expand_size) {
            expand_size += capacity_;
            if (BIT_SELECT(settings_, 0)) {
                memptr_ = allocator_.reallocate(memptr_, capacity_, expand_size);
                capacity_ = expand_size;
                return true;
            }
            return false;
        }
        /* Call this function only when you need to release the memory data before UniquePool destruction automatically. When UniquePool is being
         * destructed, it will release the memory pointer if the auto_free flag in settings_ is true.*/
        void release() {
            if (BIT_SELECT(settings_, 1) && memptr_ != nullptr) {
                allocator_.recycle(memptr_, capacity_);
                memptr_ = nullptr;
            }
        }
        ~UniquePool() {
            release();
        }


        template<typename T> requires IsSimpleType<T>
        Nexus::Utils::MayFail<T> next()  {
            constexpr auto step = sizeof(T);
            flag_ = flag_t::normal;
            if (position_ + step >= limit_) {
                flag_ = flag_t::eof;
                if (position_ + step > limit_) return Nexus::Utils::failed;
            }
            T d{};
            memcpy(&d, memptr_ + position_, step);
            position_ += step;
            return d;
        }
        template<typename T> requires IsSimpleType<T>
        bool next(T t) {
            constexpr auto step = sizeof(T);
            flag_ = flag_t::normal;
            if (position_ + step > capacity_) {
                if (BIT_SELECT(settings_, 0)) {
                    expand(step > single_automatic_expand_length ? step : single_automatic_expand_length);
                } else {
                    flag_ = flag_t::eof;
                    return false;
                }
            }
            if (position_ + step == capacity_) {
                flag_ = flag_t::eof;
            }
            memcpy(memptr_ + position_, &t, step);
            limit_ += step;
            position_ += step;
            return true;
        }
        template<typename T, size_t S> requires IsSimpleType<T>
        Nexus::Utils::MayFail<T(&)[S]> next() {
            constexpr auto size = sizeof(T) * S;
            flag_ = flag_t::normal;
            if (position_ + size >= limit_) {
                flag_ = flag_t::eof;
                if (position_ + size > limit_) return Nexus::Utils::failed;
            }
            T d{};
            memcpy(&d[0], memptr_ + position_, size);
            position_ += size;
            return d;
        }
        template<typename T, size_t S> requires IsSimpleType<T>
        bool next(T (&t)[S]) {
            constexpr auto size = sizeof(T) * S;
            flag_ = flag_t::normal;
            if (position_ + size > capacity_) {
                if (BIT_SELECT(settings_, 0)) {
                    expand(size > single_automatic_expand_length ? size : single_automatic_expand_length);
                } else {
                    flag_ = flag_t::eof;
                    return false;
                }
            }
            if (position_ + size == capacity_) {
                flag_ = flag_t::eof;
            }
            memcpy(memptr_ + position_, &t[0], size);
            limit_ += size;
            position_ += size;
        }

        void rewind() {
            position_ = 0;
        }

        uint64_t& position() {
            return position_;
        }

        void position(uint64_t npos) {
            position_ = npos;
        }

        uint64_t limit() {
            return limit_;
        }

        flag_t flag() {
            return flag_;
        }

        void close() {
            release();
        }
    };

    /*
     * SharedPool provides a safe container to manage memory, which ensures that only one running thread can hold it at any time.
     * To ensure flexibility, this class provides an optional template parameter A for using different memory allocation methods.
     * */
    template<typename A = HeapAllocator> requires IsAllocator<A>
    class SharedPool {
    public:
        static constexpr uint64_t single_automatic_expand_length = 1024;
        // Bit Assign: [7:6] Auto Expand [6:0] Reserved
        using settings = char;
        enum class flag_t {
            normal,
            eof
        };
    private:
        char** memholder_ {nullptr};
        uint64_t* capacity_ {nullptr};
        uint64_t* limit_ {nullptr};
        int64_t* reference_counting {nullptr};
        A allocator_;
        // Bit Assign: [7:1] Reserved [1:0] Auto Expand
        settings settings_{0b00000001};
        uint64_t position_{0};
        std::shared_mutex* mtx;
        flag_t flag_ {flag_t::normal};
    public:
        /* Allocate a new SharedPool with given capacity. */
        explicit SharedPool(uint64_t capacity) : allocator_(A()), mtx(new std::shared_mutex), settings_(1) {
            reference_counting = reinterpret_cast<int64_t*>(allocator_.allocate(sizeof(int64_t)));
            *reference_counting = 1;
            limit_ = reinterpret_cast<uint64_t*>(allocator_.allocate(sizeof(uint64_t)));
            *limit_ = 0;
            capacity_ = reinterpret_cast<uint64_t*>(allocator_.allocate(sizeof(uint64_t)));
            *capacity_ = capacity;
            memholder_ = reinterpret_cast<char**>(allocator_.allocate(sizeof(char**)));
            *memholder_ = allocator_.allocate(capacity);
        }

        /* Use SharedPool to manage a pointer and carefully confirm the life cycle of the pointer. */
        SharedPool(char* memptr, uint64_t size) : allocator_(A()), mtx(new std::shared_mutex) {
            reference_counting = reinterpret_cast<int64_t*>(allocator_.allocate(sizeof(int64_t)));
            *reference_counting = 1;
            limit_ = reinterpret_cast<uint64_t*>(allocator_.allocate(sizeof(uint64_t)));
            *limit_ = size;
            capacity_ = reinterpret_cast<uint64_t*>(allocator_.allocate(sizeof(uint64_t)));
            *capacity_ = size;
            memholder_ = reinterpret_cast<char**>(allocator_.allocate(sizeof(char**)));
            *memholder_ =  memptr;
        }

        /* SharedPool can be copied. */
        SharedPool(const SharedPool& up) : allocator_(up.allocator_), capacity_(up.capacity_), limit_(up.limit_), memholder_(up.memholder_), mtx(up.mtx), settings_((up.settings_)), reference_counting(up.reference_counting) {
            if (!adjust_refcount(1)) {
                throw std::runtime_error("Coping SharedPool when being destructed.");
            }
        }

        /* SharedPool cannot be moved. */
        SharedPool(SharedPool&& up) noexcept : allocator_(up.allocator_), capacity_(up.capacity_), limit_(up.limit_), memholder_(up.memholder_), mtx(up.mtx), settings_((up.settings_)), reference_counting(up.reference_counting) {
            up.memholder_ = nullptr;
        }

        /* Direct access to the buffer. */
        char& operator[](uint64_t index) {
            return (*memholder_)[index];
        }
        /* Read data in specified size with specified position. This function wouldn't set any flag */
        Nexus::Utils::MayFail<UniqueFlexHolder<char>> read(uint64_t off, uint64_t len) {
            mtx->lock_shared();
            if ((off + len > *capacity_)) {
                mtx->unlock_shared();
                return Nexus::Utils::failed;
            }
            auto data = UniqueFlexHolder<char>(len);
            memcpy(&data.get(), *memholder_ + off, len);
            mtx->unlock_shared();
            return data;
        }

        /* Read data in specified size with position. */
        Nexus::Utils::MayFail<UniqueFlexHolder<char>> read(uint64_t len) {
            mtx->lock_shared();
            flag_ = flag_t::normal;
            if (position_ >= *limit_) {
                flag_ = flag_t::eof;
                mtx->unlock_shared();
                return Nexus::Utils::failed;
            }
            if (position_ + len >= *limit_) {
                len = *limit_ - position_;
            }
            auto data = UniqueFlexHolder<char>(len);
            memcpy(&data.get(), *memholder_ + position_, len);
            position_ += len;
            mtx->unlock_shared();
            return data;
        }

        /* Write data in specified size with specified position. */
        bool write(const char* ptr, uint64_t off, uint64_t len) {
            mtx->lock();
            if (off + len > *capacity_) {
                if (BIT_SELECT(settings_, 0)) {
                    expand((off + len - *capacity_) > single_automatic_expand_length ? (off + len - *capacity_) : single_automatic_expand_length);
                } else {
                    mtx->unlock();
                    return false;
                }
            }
            memcpy(*memholder_ + off, ptr, len);
            mtx->unlock();
            return true;
        }

        /* Write data in specified size with position. */
        bool write(const char* ptr, uint64_t len) {
            mtx->lock();
            flag_ = flag_t::normal;
            if (position_ + len > *capacity_) {
                if (BIT_SELECT(settings_, 0)) {
                    expand((position_ + len - *capacity_) > single_automatic_expand_length ? (position_ + len - *capacity_) : single_automatic_expand_length);
                } else {
                    flag_ = flag_t::eof;
                    mtx->unlock();
                    return false;
                }
            }
            if (position_ + len == *capacity_) flag_ = flag_t::eof;
            memcpy(*memholder_ + position_, ptr, len);
            *limit_ += len;
            position_ += len;
            mtx->unlock();
            return true;
        }

        /* Apply settings for UniquePool */
        void apply_settings(const settings& settings) {
            settings_ = settings;
        }
        /* Adjust the reference counter; Pass positive value to increase the value and negative value to decrease the value. */
        bool adjust_refcount(int refcount) {
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

        /* Make sure Mutex is locked before expand size */
        bool expand(uint64_t new_capacity) {
            new_capacity += *capacity_;
            if (BIT_SELECT(settings_, 0)) {
                *memholder_ = allocator_.reallocate(*memholder_, *capacity_, new_capacity);
                *capacity_ = new_capacity;
                return true;
            }
            return false;
        }

        /* Call this function only when you need to release the memory data before SharedPool destruction automatically. When SharedPool is being
         * destructed, it will release the memory pointer if the auto_free flag in settings_ is true.*/
        void release() {
            if (memholder_ != nullptr) {
                if (!adjust_refcount(-1)) {
                    allocator_.recycle(*memholder_, *capacity_);
                    allocator_.recycle(reinterpret_cast<char*>(memholder_), sizeof(char*));
                    memholder_ = nullptr;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // wait 10ms to make sure all the conflicting threads could pass.
                    delete mtx;
                    mtx = nullptr;
                    allocator_.recycle(reinterpret_cast<char*>(reference_counting), sizeof(int64_t));
                    allocator_.recycle(reinterpret_cast<char*>(capacity_), sizeof(uint64_t));
                    allocator_.recycle(reinterpret_cast<char*>(limit_), sizeof(uint64_t));
                    reference_counting = nullptr;
                    capacity_ = nullptr;
                    limit_ = nullptr;
                }
                memholder_ = nullptr;
            }
        }
        ~SharedPool() {
            release();
        }


        template<typename T> requires IsSimpleType<T>
        Nexus::Utils::MayFail<T> next() {
            constexpr auto step = sizeof(T);
            mtx->lock_shared();
            flag_ = flag_t::normal;
            if (position_ + step >= *limit_) {
                flag_ = flag_t::eof;
                if (position_ + step > *limit_) {
                    mtx->unlock_shared();
                    return Nexus::Utils::failed;
                }
            }
            T d{};
            memcpy(&d, *memholder_ + position_, step);
            position_ += step;
            mtx->unlock_shared();
            return d;
        }
        template<typename T> requires IsSimpleType<T>
        bool next(T t) {
            constexpr auto step = sizeof(T);
            mtx->lock();
            flag_ = flag_t::normal;
            if (position_ + step > *capacity_) {
                if (BIT_SELECT(settings_, 0)) {
                    expand(step > single_automatic_expand_length ? position_ + step : position_ + single_automatic_expand_length);
                } else {
                    flag_ = flag_t::eof;
                    mtx->unlock();
                    return false;
                }
            }
            if (position_ + step == *capacity_) flag_ = flag_t::eof;
            memcpy(*memholder_ + position_, &t, step);
            *limit_ += step;
            position_ += step;
            mtx->unlock();
            return true;
        }
        template<typename T, size_t S> requires IsSimpleType<T>
        Nexus::Utils::MayFail<T[S]> next() {
            constexpr auto size = sizeof(T) * S;
            mtx->lock_shared();
            flag_ = flag_t::normal;
            if (position_ + size >= *limit_) {
                flag_ = flag_t::eof;
                if (position_ + size > *limit_) {
                    mtx->unlock_shared();
                    return Nexus::Utils::failed;
                }
            }
            T d{};
            memcpy(&d[0], *memholder_ + position_, size);
            position_ += size;
            mtx->unlock_shared();
            return d;
        }
        template<typename T, size_t S> requires IsSimpleType<T>
        bool next(T(&t)[S]) {
            constexpr auto size = sizeof(T) * S;
            mtx->lock();
            flag_ = flag_t::normal;
            if (position_ + size > *capacity_) {
                if (BIT_SELECT(settings_, 0)) {
                    expand(size > single_automatic_expand_length ? position_ + size : position_ + single_automatic_expand_length);
                } else {
                    flag_ = flag_t::eof;
                    mtx->unlock();
                    return false;
                }
            }
            if (position_ + size == *capacity_) flag_ = flag_t::eof;;
            memcpy(*memholder_ + position_, &t[0], size);
            *limit_ += size;
            position_ += size;
            mtx->unlock();
            return true;
        }
        void rewind() {
            position_ = 0;
        }

        uint64_t& position() {
            return position_;
        }

        void position(uint64_t npos) {
            position_ = npos;
        }

        uint64_t limit() {
            return *limit_;
        }

        flag_t flag() {
            return flag_;
        }
        void close() {
            release();
        }
    };
}