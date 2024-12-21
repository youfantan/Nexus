#pragma once
#include <cstdint>
#include <concepts>
#include <shared_mutex>
#include "check.h"

namespace Nexus::Base {
    template<typename A>
    concept IsAllocator = requires(A a) {
        { a.allocate(UINT64_MAX) } -> std::convertible_to<char*>;
        { a.reallocate(nullptr, UINT64_MAX, UINT64_MAX) } -> std::convertible_to<char*>;
        { a.recycle(nullptr, UINT64_MAX) } -> std::same_as<bool>;
    };

    /*
     * For a container that supports streaming operations, it must provide next() function which returns data with any plain type.
     * */

    struct plain_type_for_constraint {
        uint32_t a;
        char b[14];
        float c;
        bool d;
    };

    template<typename C>
    concept ContainerStreamable = requires(C c) {
        { c.template next<plain_type_for_constraint>() } -> std::same_as<Nexus::Check::mayfail<plain_type_for_constraint>>;
        { c.template next<plain_type_for_constraint>(plain_type_for_constraint()) } -> std::same_as<bool>;
        { c.rewind() };
        { c.position() } -> std::same_as<uint64_t>;
        { c.position(UINT64_MAX) };
        { c.flag() } -> std::same_as<typename C::flag_t>;
        { c.close() };
    };

    template<typename T>
    concept IsSimpleType = std::is_fundamental_v<T> || (std::is_class_v<T> && !std::is_member_function_pointer_v<T>) || std::is_array_v<T>;

    template<typename C> requires ContainerStreamable<C>
    class Stream {
    private:
        C container_;
    public:
        /* Construct stream with a reference of container. */
        explicit Stream(const C& container);
        /* Construct stream with a right reference of container. */
        explicit Stream(C&& container);
        /* Read the next data. */
        template<typename T> requires IsSimpleType<T>
        Nexus::Check::mayfail<T> next();
        /* Write the next data. */
        template<typename T> requires IsSimpleType<T>
        bool next(T t);
        /* Rewind the container. */
        void rewind();
        /* Get the position of container. */
        uint64_t position();
        /* Set the position of container. */
        void position(uint64_t npos);
        /* Get the last operation flag of container. */
        C::flag_t flag();
        /* Call this function only when you need to release the container before Stream destruction automatically. When Stream is being
         * destructed, it will close the container.  */
        void close();
        ~Stream();
    };

    class HeapAllocator {
    public:
        char* allocate(uint64_t size);
        char* reallocate(char* old_ptr, uint64_t old_size, uint64_t new_size);
        bool recycle(char* ptr, uint64_t size);
    };

    /*
     * UniquePool provides a safe container to manage memory, which ensures that only one running thread can hold it at any time.
     * To ensure flexibility, this class provides an optional template parameter A for using different memory allocation methods.
     * */
    template<typename A = HeapAllocator> requires IsAllocator<A>
    class UniquePool {
    public:
        static constexpr uint64_t single_automatic_expand_length = 1024;
        struct settings {
            char auto_expand:1;
            char auto_free:1;
            char reserved:6;
        };
        enum class flag_t {
            eof
        };
    private:
        char* memptr_;
        uint64_t capacity_;
        A allocator_;
        settings settings_{1, 1, 0};
        uint64_t position_{0};
        flag_t flag_;
    public:
        /* Allocate a new UniquePool with given capacity. */
        explicit UniquePool(uint64_t capacity);
        /* Use UniquePool to manage a pointer and carefully confirm the life cycle of the pointer. */
        UniquePool(char* memptr, uint64_t size);
        /* UniquePool cannot be copied. */
        UniquePool(const UniquePool& up) = delete;
        /* UniquePool can be moved. */
        UniquePool(UniquePool&& up);
        /* Apply settings for UniquePool */
        void apply_settings(const settings& settings);
        bool expand(uint64_t new_capacity);
        /* Call this function only when you need to release the memory data before UniquePool destruction automatically. When UniquePool is being
         * destructed, it will release the memory pointer if the auto_free flag in settings_ is true.*/
        void release();
        ~UniquePool();


        template<typename T> requires IsSimpleType<T>
        Nexus::Check::mayfail<T> next();
        template<typename T> requires IsSimpleType<T>
        bool next(T t);
        void rewind();
        uint64_t position();
        void position(uint64_t npos);
        flag_t flag();
        void close();
    };

    /*
     * SharedPool provides a safe container to manage memory, which ensures that only one running thread can hold it at any time.
     * To ensure flexibility, this class provides an optional template parameter A for using different memory allocation methods.
     * */
    template<typename A = HeapAllocator> requires IsAllocator<A>
    class SharedPool {
    public:
        static constexpr uint64_t single_automatic_expand_length = 1024;
        struct settings {
            char auto_expand:1;
            char reserved:7;
        };
        enum class flag_t {
            eof
        };
    private:
        char** memholder_;
        uint64_t* capacity_{new uint64_t(0)};
        A allocator_;
        settings settings_{1, 0};
        uint64_t position_{0};
        int64_t* reference_counting{new int64_t (1)};
        std::shared_mutex* mtx;
        flag_t flag_;
    public:
        /* Allocate a new SharedPool with given capacity. */
        explicit SharedPool(uint64_t capacity);
        /* Use SharedPool to manage a pointer and carefully confirm the life cycle of the pointer. */
        SharedPool(char* memptr, uint64_t size);
        /* SharedPool can be copied. */
        SharedPool(const SharedPool& up);
        /* SharedPool cannot be moved. */
        SharedPool(SharedPool&& up) = delete;
        /* Apply settings for UniquePool */
        void apply_settings(const settings& settings);
        /* Adjust the reference counter; Pass positive value to increase the value and negative value to decrease the value. */
        bool adjust_refcount(int refcount);
        bool expand(uint64_t new_capacity);
        /* Call this function only when you need to release the memory data before SharedPool destruction automatically. When SharedPool is being
         * destructed, it will release the memory pointer if the auto_free flag in settings_ is true.*/
        void release();
        ~SharedPool();


        template<typename T> requires IsSimpleType<T>
        Nexus::Check::mayfail<T> next();
        template<typename T> requires IsSimpleType<T>
        bool next(T t);
        void rewind();
        uint64_t position();
        void position(uint64_t npos);
        flag_t flag();
        void close();
    };

}

#include "../src/memory.tpp"