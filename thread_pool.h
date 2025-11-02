#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#pragma once 
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <atomic>

class ThreadPool {
public:
	ThreadPool(size_t n) {
		stop = false;
		for (size_t i = 0; i < n; ++i) {
			workers.emplace_back([this]() {this->worker(); });
		}
	}

	~ThreadPool() {
		{
			std::unique_lock<std::mutex> lock(m);
			stop = true;
		}
		cv.notify_all();
		for (auto& t : workers) {
			t.join();
		}
	}

	void enqueue(std::function<void()> job) {
		{ 
			std::unique_lock<std::mutex> lock(m);
			jobs.push(std::move(job));
		}
		cv.notify_one();
	}

	void wait() {
		std::unique_lock<std::mutex> lock(m_done);
		cv_done.wait(lock, [&]() { return jobs.empty() && active == 0; });
	}

private:

	void worker() {
		for (;;) {
			std::function<void()> job;
			{
				std::unique_lock<std::mutex> lock(m);
				cv.wait(lock, [&]() {return stop || !jobs.empty(); });
				if (stop && jobs.empty()) {
					return;
				}
				job = std::move(jobs.front());
				jobs.pop();
				++active;
			}

			job();

			{
				std::unique_lock<std::mutex> lock(m_done);
				--active;
				if (jobs.empty() && active == 0) {
					cv_done.notify_one();
				}
			}
		}
	}

	std::vector<std::thread> workers;
	std::queue<std::function<void()>> jobs;
	std::mutex m, m_done;
	std::condition_variable cv, cv_done;
	std::atomic<bool> stop;
	int active = 0;

};

#endif