#pragma once
/* Because std::optional didn't support array, so created this class to solve it. */

#include <algorithm>

namespace Nexus::Utils {
    class failed_t {};

    static constexpr failed_t failed = failed_t();

    template<typename R>
    class mayfail {
    private:
        R r_;
        bool valid_;
    public:
        mayfail(R& retv) : r_(retv) {
            valid_ = true;
        }
        mayfail(R&& retv) : r_(std::move(retv)) {
            valid_ = true;
        }
        mayfail(failed_t f) {
            valid_ = false;
        }
        bool is_valid () {
            return valid_;
        }
        R& result() {
            return r_;
        }
    };

    template<typename R, size_t N>
    class mayfail<R[N]> {
    private:
        bool valid_;
        R r_[N];
    public:
        mayfail(const R (&retv)[N]) {
            for (int i = 0; i < N; ++i) {
                r_[i] = retv[i];
            }
            valid_ = true;
        }
        mayfail(failed_t f) {
            valid_ = false;
        }
        bool is_valid () {
            return valid_;
        }
        R& result() {
            return r_;
        }
    };
}