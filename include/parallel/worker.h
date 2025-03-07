#pragma once

#include <functional>
#include <array>
#include <thread>

namespace Nexus::Parallel {
    using affair = std::function<void()>;

    template<int N> requires (N >= 0)
    class WorkGroup;

    class Worker {
        template<int N> requires (N >= 0)
        friend class WorkGroup;
    private:
        std::thread worker_thread_;
        std::queue<affair>& affair_queue_;
        std::atomic<bool> flag_ { false };
        std::mutex mtx_;
        std::condition_variable cv_;

    public:
        explicit Worker(std::queue<affair>& affair_queue) : affair_queue_(affair_queue), mtx_(std::mutex{}) {}
        void start() {
            worker_thread_ = std::thread(&Worker::execute, this);
        }
        Worker(Worker&& w) noexcept : worker_thread_(std::move(w.worker_thread_)), affair_queue_(w.affair_queue_), flag_(w.flag_.load()), mtx_(std::mutex{}) {}

        void execute() {
            while (!flag_) {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [&] { return !affair_queue_.empty() || flag_; });
                if (!affair_queue_.empty()) {
                    affair_queue_.front()();
                    affair_queue_.pop();
                }
            }
        }

        void stop() {
            flag_ = true;
        }

    };

    template<int N> requires (N >= 0)
    class WorkGroup {
    private:
        std::vector<Worker> workers_;
        std::array<std::queue<affair>, N> queues_;
    public:
        WorkGroup() {
            for (int i = 0; i < N; ++i) {
                workers_.emplace_back(queues_[i]);
            }
            for (int i = 0; i < N; ++i) {
                workers_[i].start();
            }
        }
        void post(affair&& af) {
            std::array<int, N> tasks;
            for (int i = 0; i < N; ++i) {
                tasks[i] = queues_[i].size();
            }
            int i = 0;
            for (int j = 1; j < N; ++j) {
                if (queues_[i].size() > queues_[j].size()) {
                    i = j;
                }
            }
            workers_[i].mtx_.lock();
            queues_[i].push(std::forward<affair>(af));
            workers_[i].mtx_.unlock();
            workers_[i].cv_.notify_one();
        }
        void cleanup() {
            for (auto& w : workers_) {
                w.cv_.notify_one();
                w.stop();
                w.worker_thread_.join();
            }
        }
    };

    template<>
    class WorkGroup<0> {
    private:
    public:
        WorkGroup() = default;
        void post(affair&& af) {
            af();
        }
        void cleanup() {}
    };
}