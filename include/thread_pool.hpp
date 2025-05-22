//
// Created by ilya on 22.05.2025.
//

#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>


class Pool {
public:
	explicit Pool(const size_t workers = std::thread::hardware_concurrency()) :
		done_(false) {
		for (size_t i = 0; i < workers; ++i) {
			threads_.emplace_back([this] { loop(); });
		}
	}

	~Pool() {
		{
			std::unique_lock<std::mutex> lock(mutex_);
			done_ = true;
		}
		cv_.notify_all();
		for (auto &thread: threads_) {
			if (thread.joinable()) thread.join();
		}
	}

	void enqueue(std::function<void()> task) {
		{
			std::unique_lock lock(mutex_);
			tasks_.push(std::move(task));
		}
		cv_.notify_one();
	}

private:
	void loop() {
		while (true) {
			std::function<void()> task;
			{
				std::unique_lock lock(mutex_);
				cv_.wait(lock, [this] { return done_ || !tasks_.empty(); });
				if (done_ && tasks_.empty()) return;
				task = std::move(tasks_.front());
				tasks_.pop();
			}
			task();
		}
	}

	std::vector<std::thread> threads_;
	std::queue<std::function<void()> > tasks_;
	std::mutex mutex_;
	std::condition_variable cv_;
	std::atomic<bool> done_;
};
