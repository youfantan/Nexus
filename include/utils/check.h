#pragma once
/* Because std::optional didn't support array, so created this class to solve it. */

#include <algorithm>

namespace Nexus::Utils {
    class failed_t {};

    static constexpr failed_t failed = failed_t();

    template<typename R>
    class MayFail {
    private:
        R r_ {};
        bool valid_;
    public:
        MayFail(R& retv) : r_(retv) {
            valid_ = true;
        }
        MayFail(R&& retv) : r_(std::move(retv)) {
            valid_ = true;
        }
        MayFail(failed_t f) {
            valid_ = false;
        }
        bool is_valid () {
            return valid_;
        }
        R& reference() {
            return r_;
        }
        R&& result() {
            return std::move(r_);
        }
    };

    template<typename R, size_t N>
    class MayFail<R[N]> {
    private:
        bool valid_;
        R r_[N];
    public:
        MayFail(const R (&retv)[N]) {
            for (int i = 0; i < N; ++i) {
                r_[i] = retv[i];
            }
            valid_ = true;
        }
        MayFail(failed_t f) {
            valid_ = false;
        }
        bool is_valid () {
            return valid_;
        }
        R(&reference())[N] {
            return r_;
        }
    };
}