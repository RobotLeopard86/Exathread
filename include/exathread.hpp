/**
 * @file exathread.hpp
 * @author Created by RobotLeopard86
 * @version Version 1.0.0
 * @copyright Copyright (c) 2025 RobotLeopard86, licensed under the Apache License 2.0
 */

/*
Copyright (c) 2025 RobotLeopard86

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

----------------------------------------------------------------------------------

"But I'm not a lawyer? What does this mean?"
Good question. Well hello there, it's me, RobotLeopard86.
Just as a note, I've found the Apache License 2.0 to be fairly readable for regular humans,
but basically it breaks down to this:
(please note, I am not a lawyer, and while I have tried to be as accurate as possible here,
this is not a substitute for the License itself nor is this legal advice, so please go read the License for full details)

1. You can use Exathread for whatever you want, for free, anywhere in the world, forever (I can't stop you if I wanted)
2. Neither myself nor any Exathread contributors can sue each other or you over patent claims on Exathread we may have
3. If you, myself, or any Exathread contributors try to sue each other over patent claims, they don't get to use Exathread anymore
4. You can copy and (re)distribute Exathread, even if you changed it, in any form, as long as you also provide a copy of the License,
	   don't get rid of the copyright notices, and if you changed it, note that you did
5. If you contribute to Exathread, those contributions are now subject to the License
6. Like it says above, Exathread is distributed as-is with no warranty, so I don't have to help you with anything (but I'll try to, I'm nice)
7. If Exathread breaks your stuff, it's not my fault or any contributor's fault
8. If you offer support or a warranty for your Exathread distribution, you can't hold myself nor any contributor responsible
*/

#pragma once

#include <any>
#include <atomic>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <ranges>
#include <stop_token>
#include <sys/wait.h>
#include <thread>
#include <type_traits>
#include <vector>
#include <utility>
#include <optional>
#include <functional>
#include <stdexcept>
#include <chrono>

///@brief The root namespace for all Exathread functionality
namespace exathread {
	class Pool;

	/**
	 * @brief The current state of a future
	 */
	enum class Status {
		Pending,  ///<The task has not yet been scheduled for execution
		Scheduled,///<The task has been scheduled for execution but has not begun
		Executing,///<The task is currently executing
		Yielded,  ///<The task has yielded temporarily
		Failed,	  ///<The task completed with an exception
		Complete  ///<The task completed successfully
	};

	///@cond
	namespace details {
		struct Promise;
		struct VoidPromise;
		template<typename T>
			requires(!std::is_void_v<T>)
		struct ValuePromise;
		struct YieldOp;
		struct ThreadData;
		template<typename... Args>
		struct ArgsHolder;
	}
	///@endcond

	/**
	 * @brief Base coroutine handle management class
	 *
	 * @warning Do not interface with this class directly; it is documented but is not meant for general use
	 */
	class Task {
	  private:
		std::coroutine_handle<details::Promise> h;

	  public:
		/**
		 * @brief Create a blank task
		 */
		Task() = default;

		/**
		 * @brief Create a task managing a coroutine
		 */
		explicit Task(std::coroutine_handle<details::Promise> h) : h(h) {}

		/**
		 * @brief Copy construction
		 */
		Task(const Task& other) noexcept;

		/**
		 * @brief Copy assignment
		 */
		Task& operator=(const Task& other) noexcept;

		/**
		 * @brief Move construction
		 */
		Task(Task&& other) noexcept : h(std::exchange(other.h, {})) {}

		/**
		 * @brief Move assignment
		 */
		Task& operator=(Task&& other) noexcept;

		~Task() noexcept;

		/**
		 * @brief Check if a task has completed execution
		 *
		 * @return Completion state
		 */
		bool done() const noexcept {
			return !h || h.done();
		}

		/**
		 * @brief Resume execution of a task
		 *
		 * @throws std::logic_error If the task is done
		 */
		void resume() {
			if(done()) throw std::logic_error("Cannot resume a done task!");
			h.resume();
		}

		/**
		 * @brief Access the underlying promise
		 *
		 * @return The promise data
		 */
		details::Promise& promise() noexcept {
			return h.promise();
		}

		/**
		 * @brief Access the underlying promise
		 *
		 * @return The promise data
		 */
		const details::Promise& promise() const noexcept {
			return h.promise();
		}

		/**
		 * @brief Access the handle to the managed coroutine
		 *
		 * @return The coroutine handle
		 */
		std::coroutine_handle<details::Promise> handle() noexcept {
			return h;
		}
	};

	/**
	 * @brief Coroutine return type for void-returning functions
	 *
	 * @note Set this as your submitted function's return type if it returns @c void and wants to use yield operations
	 */
	class VoidTask : public Task {
	  public:
		using promise_type = details::VoidPromise;

		explicit VoidTask(std::coroutine_handle<details::Promise> h) : Task(h) {}
		VoidTask(Task&& t) : Task(std::move(t)) {}
	};

	/**
	 * @brief Coroutine return type for value-returning functions
	 *
	 * @note Set this as your submitted function's return type if it returns something other than @c void and wants to use yield operations
	 *
	 * @tparam The return type of your function (this has no effect and only exists for readability purposes)
	 */
	template<typename T>
		requires(!std::is_void_v<T>)
	class ValueTask : public Task {
	  public:
		using promise_type = details::ValuePromise<T>;

		explicit ValueTask(std::coroutine_handle<details::Promise> h) : Task(h) {}
		ValueTask(Task&& t) : Task(std::move(t)) {}
	};

	template<typename T = void>
	class Future;

	template<typename T = void>
	class MultiFuture;

	/**
	 * @brief A task in a given pool which will eventually resolve to a result
	 */
	template<typename T>
	class Future {
	  public:
		/**
		 * @brief Block until execution has completed, successfully or not
		 */
		void await();

		/**
		 * @brief Get the status of a future
		 *
		 * @returns The future's current status
		 */
		Status checkStatus() const noexcept;

		/**
		 * @brief Schedule a tracked task for execution after this future
		 *
		 * @tparam F Function type
		 * @tparam ExArgs Arguments to the function
		 * @tparam R Function return type
		 *
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 *
		 * @throws std::logic_error If the future has been completed
		 * @throws std::bad_weak_ptr If the pool to which this future belongs no longer exists
		 */
		template<typename F, typename... ExArgs, typename R = std::invoke_result_t<F&&, const T&, ExArgs&&...>>
			requires std::invocable<F&&, const T&, ExArgs&&...>
		[[nodiscard]] Future<R> then(F func, ExArgs... exargs);

		/**
		 * @brief Schedule a tracked task for execution after this future with no result
		 *
		 * @tparam F Function type
		 * @tparam ExArgs Extra arguments to the function
		 *
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 *
		 * @throws std::logic_error If the future has been completed
		 * @throws std::bad_weak_ptr If the pool to which this future belongs no longer exists
		 */
		template<typename F, typename... ExArgs>
			requires std::invocable<F&&, const T&, ExArgs&&...> && std::is_void_v<std::invoke_result_t<F&&, const T&, ExArgs&&...>>
		void thenDetached(F func, ExArgs... exargs);

		/**
		 * @brief Schedule a tracked batch job based on a container for execution after this future
		 *
		 * @tparam Rn Source data container type
		 * @tparam I Type of elements in container
		 * @tparam F Function type
		 * @tparam ExArgs Extra arguments to the function
		 * @tparam R Function return type
		 *
		 * @param src The source range to iterate over
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 *
		 * @throws std::logic_error If the future has been completed
		 * @throws std::bad_weak_ptr If the pool to which this future belongs no longer exists
		 */
		template<std::ranges::input_range Rn, typename F, typename... ExArgs, typename I = std::ranges::range_value_t<Rn>, typename R = std::invoke_result_t<F&&, const T&, const I&, ExArgs&&...>>
			requires std::invocable<F&&, const T&&, I&, ExArgs&&...>
		[[nodiscard]] MultiFuture<R> thenBatch(Rn&& src, F func, ExArgs... exargs);

		/**
		 * @brief Schedule a batch job based on a container for execution after this future with no result
		 *
		 * @tparam Rn Source data container type
		 * @tparam I Type of elements in container
		 * @tparam F Function type
		 * @tparam ExArgs Extra arguments to the function
		 *
		 * @param src The source range to iterate over
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 *
		 * @throws std::logic_error If the future has been completed
		 * @throws std::bad_weak_ptr If the pool to which this future belongs no longer exists
		 */
		template<std::ranges::input_range Rn, typename F, typename... ExArgs, typename I = std::ranges::range_value_t<Rn>>
			requires std::invocable<F&&, const T&&, I&, ExArgs&&...> && std::is_void_v<std::invoke_result_t<F&&, const T&&, I&, ExArgs&&...>>
		void thenBatchDetached(Rn&& src, F func, ExArgs... exargs);

		/**
		 * @brief Obtain the task result, blocking if not complete
		 *
		 * This function does not exist in the @c void specialization of this type
		 *
		 * @return The result of the task
		 *
		 * @throws The exception thrown by the task if failed
		 */
		std::enable_if_t<!std::is_void_v<T>, T&> operator*();

		/**
		 * @brief Obtain the task result, blocking if not complete
		 *
		 * This function does not exist in the @c void specialization of this type
		 *
		 * @return The result of the task
		 *
		 * @throws The exception thrown by the task if failed
		 */
		std::enable_if_t<!std::is_void_v<T>, const T&> operator*() const;

		/**
		 * @brief Obtain the task result, blocking if not complete
		 *
		 * This function does not exist in the @c void specialization of this type
		 *
		 * @return The result of the task
		 *
		 * @throws The exception thrown by the task if failed
		 */
		std::enable_if_t<!std::is_void_v<T>, T*> operator->();

		//Move only
		///@cond
		Future(const Future&) = delete;
		Future& operator=(const Future&) = delete;
		Future(Future&&) = default;
		Future& operator=(Future&&) = default;
		///@endcond
	  private:
		Task task;

		Future() {}
		friend class Pool;
	};

	/**
	 * @brief Aggregate container of multiple futures
	 */
	template<typename T>
	class MultiFuture {
	  public:
		/**
		 * @brief Create a MultiFuture with a collection of futures
		 *
		 * @throws std::logic_error If any of the futures belongs to a different pools from the others
		 */
		explicit MultiFuture(Future<T>, ...);

		/**
		 * @brief Get the number of collected futures
		 *
		 * @returns Future count
		 */
		std::size_t size() const noexcept;

		/**
		 * @brief Block until all futures have completed execution
		 */
		void await();

		/**
		 * @brief Get the overall status of the collection
		 *
		 * @details The status progresses as follows: starting at Pending (then Scheduled once scheduled to run), once any future starts executing the status is set to Executing, then Complete once all futures have completed.\n The status will be set to Failed if any future fails. The Yielding status is never returned.
		 *
		 * @returns The overall status
		 */
		Status checkStatus() const noexcept;

		/**
		 * @brief Schedule a tracked task for execution after these futures
		 *
		 * @tparam F Function type
		 * @tparam ExArgs Arguments to the function
		 * @tparam R Function return type
		 *
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 *
		 * @throws std::logic_error If the future has already been completed
		 * @throws std::bad_weak_ptr If the pool to which the futures belong no longer exists
		 */
		template<typename F, typename... ExArgs, typename R = std::invoke_result_t<F&&, const std::vector<T>&, ExArgs&&...>>
			requires std::invocable<F&&, const std::vector<T>&, ExArgs&&...>
		[[nodiscard]] Future<R> then(F func, ExArgs... exargs);

		/**
		 * @brief Schedule a tracked task for execution after these futures with no result
		 *
		 * @tparam F Function type
		 * @tparam ExArgs Extra arguments to the function
		 *
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 *
		 * @throws std::logic_error If the future has already been completed
		 * @throws std::bad_weak_ptr If the pool to which the futures belong no longer exists
		 */
		template<typename F, typename... ExArgs>
			requires std::invocable<F&&, const std::vector<T>&, ExArgs&&...> && std::is_void_v<std::invoke_result_t<F&&, const std::vector<T>&, ExArgs&&...>>
		void thenDetached(F func, ExArgs... exargs);

		/**
		 * @brief Schedule a tracked batch job based on a container for execution after these futures
		 *
		 * @tparam F Function type
		 * @tparam ExArgs Extra arguments to the function
		 * @tparam R Function return type
		 *
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 *
		 * @throws std::logic_error If the future has been completed
		 * @throws std::bad_weak_ptr If the pool to which the futures belong no longer exists
		 */
		template<typename F, typename... ExArgs, typename R = std::invoke_result_t<F&&, const T&, ExArgs&&...>>
			requires std::invocable<F&&, const T&, ExArgs&&...>
		[[nodiscard]] MultiFuture<R> thenBatch(F func, ExArgs... exargs);

		/**
		 * @brief Schedule a batch job based on a container for execution after these futures with no result
		 *
		 * @tparam F Function type
		 * @tparam ExArgs Extra arguments to the function
		 *
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 *
		 * @throws std::logic_error If the future has been completed
		 * @throws std::bad_weak_ptr If the pool to which the futures belong no longer exists
		 */
		template<typename F, typename... ExArgs>
			requires std::invocable<F&&, const T&, ExArgs&&...> && std::is_void_v<std::invoke_result_t<F&&, const T&, ExArgs&&...>>
		void thenBatchDetached(F func, ExArgs... exargs);

		/**
		 * @brief Get the results of the futures, blocking if not complete
		 *
		 * This function does not exist in the @c void specialization of this type
		 *
		 * @returns A list of results corresponding to the order of futures as placed in the constructor
		 *
		 * @throws std::runtime_error If any of the futures failed
		 */
		std::enable_if_t<!std::is_void_v<T>, std::vector<T>> results();

	  private:
		std::vector<Future<T>> futures;
	};

	/**
	 * @brief A group of threads to execute tasks
	 */
	class Pool : public std::enable_shared_from_this<Pool> {
	  public:
		/**
		 * @brief Create a new pool with a set amount of threads
		 *
		 * @param threadCount The number of threads to assign to the pool (half of the hardware concurrency by default)
		 *
		 * @throws std::out_of_range If the amount of threads consumed by all pools would exceed std::thread::hardware_concurrency if this pool were created
		 */
		static std::shared_ptr<Pool> Create(std::size_t threadCount = std::thread::hardware_concurrency() / 2) {
			return std::shared_ptr<Pool>(new Pool(threadCount));
		}

		///@cond
		Pool(const Pool&) = delete;
		Pool& operator=(const Pool&) = delete;
		Pool(Pool&&) = delete;
		Pool& operator=(Pool&&) = delete;
		///@endcond

		/**
		 * @brief Submit a tracked task into the pool
		 *
		 * @tparam F Function type
		 * @tparam Args Arguments to the function
		 * @tparam R Function return type
		 *
		 * @param func The function to invoke
		 * @param args Arguments to pass to the function
		 */
		template<typename F, typename... Args, typename R = std::invoke_result_t<F&&, Args&&...>>
			requires std::invocable<F&&, Args&&...>
		[[nodiscard]] Future<R> submit(F func, Args... args);

		/**
		 * @brief Submit a task into the pool with no result
		 *
		 * @tparam F Function type
		 * @tparam Args Arguments to the function
		 *
		 * @param func The function to invoke
		 * @param args Arguments to pass to the function
		 */
		template<typename F, typename... Args>
			requires std::invocable<F&&, Args&&...> && std::is_void_v<std::invoke_result_t<F&&, Args&&...>>
		void submitDetached(F func, Args... args);

		/**
		 * @brief Submit a tracked batch job based on a container into the pool
		 *
		 * @tparam Rn Source data container type
		 * @tparam I Type of elements in container
		 * @tparam F Function type
		 * @tparam ExArgs Extra arguments to the function
		 * @tparam R Function return type
		 *
		 * @param src The source range to iterate over
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 */
		template<std::ranges::input_range Rn, typename F, typename... ExArgs, typename I = std::ranges::range_value_t<Rn>, typename R = std::invoke_result_t<F&&, const I&, ExArgs&&...>>
			requires std::invocable<F&&, const I&, ExArgs&&...>
		[[nodiscard]] MultiFuture<R> batch(const Rn& src, F func, ExArgs... exargs);

		/**
		 * @brief Submit a batch job based on a container into the pool with no result
		 *
		 * @tparam Rn Source data container type
		 * @tparam I Type of elements in container
		 * @tparam F Function type
		 * @tparam ExArgs Extra arguments to the function
		 *
		 * @param src The source range to iterate over
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 */
		template<std::ranges::input_range Rn, typename F, typename... ExArgs, typename I = std::ranges::range_value_t<Rn>>
			requires std::invocable<F&&, const I&, ExArgs&&...> && std::is_void_v<std::invoke_result_t<F&&, const I&, ExArgs&&...>>
		void batchDetached(const Rn& src, F func, ExArgs... exargs);

		/**
		 * @brief Get the number of worker threads managed the pool
		 *
		 * @returns Pool thread count
		 */
		std::size_t getThreadCount() const noexcept;

		/**
		 * @brief Wait until there are no more tasks in the queue (tasks submitted during this call will continue to block it)
		 *
		 * @note This function is only approximant due to the nature of multithreading; it is possible that some tasks may remain if they were submitted after a thread's queue was checked
		 */
		void waitIdle() const noexcept;

		~Pool();

	  private:
		Pool(std::size_t threadCount);

		static std::size_t totalThreads;
		std::vector<details::ThreadData> threads;

		friend void worker(std::stop_token, std::shared_ptr<Pool>, std::size_t);
		friend struct details::YieldOp;
		friend struct details::VoidPromise;
		template<typename U>
			requires(!std::is_void_v<U>)
		friend struct details::ValuePromise;

		void internalTaskEnqueue(Task t);
		void internalBatchEnqueue(std::vector<Task> b);
	};

	/**
	 * @brief Suspend execution of your task and allow other tasks to run for a certain period of time
	 *
	 * @note As the use of this function makes your function a coroutine, it must explicitly return either a VoidTask or ValueTask and use @c co_return to be valid
	 *
	 * @param duration The amount of time to yield for. It is not guaranteed that execution will resume exactly at the specified time amount
	 *
	 * @return An awaitable object; you must use @c co_await on this result to yield correctly
	 */
	template<typename Rep, typename Period>
	[[nodiscard]] details::YieldOp yieldFor(const std::chrono::duration<Rep, Period>& duration);

	/**
	 * @brief Suspend execution of your task and allow other tasks to run until a certain point in time
	 *
	 * @note As the use of this function makes your function a coroutine, it must explicitly return either a VoidTask or ValueTask and use @c co_return to be valid
	 *
	 * @param time The point in time until which to yield. It is not guaranteed that execution will resume exactly at the specified time point
	 *
	 * @return An awaitable object; you must use @c co_await on this result to yield correctly
	 *
	 * @throws std::logic_error If the specified time point is in the past
	 */
	[[nodiscard]] details::YieldOp yieldUntil(std::chrono::steady_clock::time_point time);

	/**
	 * @brief Suspend execution of your task and allow other tasks to run until a certain condition is met
	 *
	 * @note As the use of this function makes your function a coroutine, it must explicitly return either a VoidTask or ValueTask and use @c co_return to be valid
	 *
	 * @param predicate A function that will evaluate the condition to determine if execution should resume. It should not have side effects.
	 *
	 * @return An awaitable object; you must use @c co_await on this result to yield correctly
	 */
	[[nodiscard]] details::YieldOp yieldUntilTrue(std::function<bool()> predicate);

	/**
	 * @brief Suspend execution of your task and allow other tasks to run until a future resolves
	 *
	 * @note As the use of this function makes your function a coroutine, it must explicitly return either a VoidTask or ValueTask and use @c co_return to be valid
	 *
	 * @param future The future of which to yield until completion. It is not guaranteed that execution will resume exactly when the future becomes complete.
	 *
	 * @return An awaitable object; you must use @c co_await on this result to yield correctly
	 *
	 * @throws std::logic_error If the specified future has already been completed
	 */
	template<typename T>
	[[nodiscard]] details::YieldOp yieldUntilComplete(Future<T> future);

	/**
	 * @brief Suspend execution of your task and allow other tasks to run until futures resolve
	 *
	 * @note As the use of this function makes your function a coroutine, it must explicitly return either a VoidTask or ValueTask and use @c co_return to be valid
	 *
	 * @param futures The futures of which to yield until completion. It is not guaranteed that execution will resume exactly when the futures become complete.
	 *
	 * @return An awaitable object; you must use @c co_await on this result to yield correctly
	 *
	 * @throws std::logic_error If the specified futures have already been completed
	 */
	template<typename T>
	[[nodiscard]] details::YieldOp yieldUntilComplete(MultiFuture<T> futures);
}

//=============== IMPLEMENTATION ===============

namespace exathread {
	struct details::Promise {
		std::exception_ptr exception;		  //The stored result exception (if one is thrown)
		Status status;						  //The status of the task
		std::weak_ptr<Pool> pool;			  //The pool of execution
		std::size_t threadIdx;				  //The index of the thread this task is running on
		std::atomic_uint handleRefCount;	  //Reference count for how many tasks maintain the coroutine handle
		std::vector<std::vector<Task>> next;  //The next task(s) to schedule after the completion of this one
		std::function<void(std::any)> arg1Set;//The setter for the first argument (used for late-binding for continuations)

		void unhandled_exception() noexcept {
			exception = std::current_exception();
		}

		std::suspend_always initial_suspend() noexcept {
			status = Status::Pending;
			return {};
		}

		std::suspend_always final_suspend() noexcept {
			//Set status
			status = exception ? Status::Failed : Status::Complete;

			//Schedule continuations if success and pool still okay (double-check)
			if(!exception && !pool.expired()) {
				continuation();
			}

			return {};
		}

		virtual Task get_return_object() noexcept {
			return {};
		}

	  private:
		virtual void continuation() {}
	};

	struct details::VoidPromise : public Promise {
		void return_void() noexcept {}

		Task get_return_object() noexcept override {
			return VoidTask {std::coroutine_handle<details::Promise>::from_promise(*this)};
		}

		void continuation() override {
			std::shared_ptr<Pool> p = pool.lock();
			for(const std::vector<Task>& t : next) {
				if(t.size() == 1) {
					p->internalTaskEnqueue(t[0]);
				} else {
					p->internalBatchEnqueue(t);
				}
			}
		}
	};

	template<typename T>
		requires(!std::is_void_v<T>)
	struct details::ValuePromise : public Promise {
		std::optional<T> val;

		void return_value(T value) noexcept(std::is_nothrow_move_constructible_v<T>) {
			val = std::move(value);
		}

		Task get_return_object() noexcept override {
			return ValueTask<T> {std::coroutine_handle<details::Promise>::from_promise(*this)};
		}

		void continuation() override {
			std::shared_ptr<Pool> p = pool.lock();
			for(std::vector<Task>& t : next) {
				if(t.size() == 1) {
					t[0].promise().arg1Set(val.value());
					p->internalTaskEnqueue(t[0]);
				} else {
					for(Task& task : t) {
						task.promise().arg1Set(val.value());
					}
					p->internalBatchEnqueue(t);
				}
			}
		}
	};

	inline Task::Task(const Task& other) noexcept : h(other.h) {
		promise().handleRefCount++;
	}

	inline Task& Task::operator=(const Task& other) noexcept {
		if(this != &other) {
			h = other.h;
			promise().handleRefCount++;
		}
		return *this;
	}

	inline Task& Task::operator=(Task&& other) noexcept {
		if(this != &other) {
			h = std::exchange(other.h, {});
		}
		return *this;
	}

	inline Task::~Task() noexcept {
		if(promise().handleRefCount-- <= 0) {
			h.destroy();
		}
	}

	struct details::ThreadData {
		std::jthread thread;
		std::vector<details::YieldOp> yields;
		std::weak_ptr<Pool> pool;
		std::size_t curSteal = 0, myIndex;
		std::array<Task, 2048> ringbuf;
		alignas(64) std::atomic<uint64_t> frontHead;
		alignas(64) std::atomic<uint64_t> frontTail;
		alignas(64) std::atomic<uint64_t> backHead;
		alignas(64) std::atomic<uint64_t> backTail;
		void push(Task&& t);
		Task pop();
		std::size_t queueSize() const;

		ThreadData(ThreadData&& o);
		ThreadData& operator=(ThreadData&& o);
		ThreadData(const ThreadData&) = delete;
		ThreadData& operator=(const ThreadData&) = delete;
		ThreadData();
	};

	inline void details::ThreadData::push(Task&& t) {
		//Reserve slot in ring buffer (CAS)
		uint64_t bh0 = 0, bh1 = 0;
		do {
			//Advance indices with wrap-around protection
			uint64_t ft = frontTail.load(std::memory_order_acquire);
			bh0 = backHead.load(std::memory_order_acquire);
			bh1 = bh0 + 1;
			while((bh1 - ft) >= ringbuf.size()) {
				std::this_thread::yield();
				ft = frontTail.load(std::memory_order_acquire);
				bh0 = backHead.load(std::memory_order_acquire);
				bh1 = bh0 + 1;
			}
		} while(!backHead.compare_exchange_strong(bh0, bh1, std::memory_order_acq_rel, std::memory_order_relaxed));

		//Write to reserved slot
		ringbuf[bh1 % ringbuf.size()] = std::move(t);

		//Advance tail index when possible
		uint64_t tailBase = backTail.load(std::memory_order_relaxed);
		if(bh1 <= tailBase) return;
		while(!backTail.compare_exchange_strong(tailBase, bh1, std::memory_order_release, std::memory_order_relaxed)) {
			std::this_thread::yield();

			//Make sure the tail hasn't passed what we thought it was
			if(tailBase >= bh1) return;
		}
	}

	inline Task details::ThreadData::pop() {
		while(true) {
			//Check if empty
			if(queueSize() <= 0) throw std::runtime_error("Queue is empty!");

			//Reserve slot in ring buffer (CAS)
			uint64_t fh0 = 0, fh1 = 0;
			do {
				//Advance indices
				fh0 = frontHead.load(std::memory_order_acquire);
				fh1 = fh0 + 1;

				//Decrement prevention (tail was incremented while we were in the loop so we'd push it back if we continued)
				if(fh1 < frontTail.load(std::memory_order_acquire)) continue;

				//Wrap-around prevention (head can't wrap around past tail)
				if(fh1 - frontTail.load(std::memory_order_acquire) >= ringbuf.size()) continue;

				//Index pass prevention (front can't get ahead of back)
				if(fh1 > backTail.load(std::memory_order_acquire)) continue;
			} while(!frontHead.compare_exchange_strong(fh0, fh1, std::memory_order_acq_rel, std::memory_order_relaxed));

			//Read from safe slot
			Task t = std::move(ringbuf[fh1 % ringbuf.size()]);

			//Advance tail index when possible
			uint64_t tailBase = frontTail.load(std::memory_order_relaxed);
			if(fh1 <= tailBase) return t;
			while(!frontTail.compare_exchange_strong(tailBase, fh1, std::memory_order_release, std::memory_order_relaxed)) {
				std::this_thread::yield();

				//Make sure the tail hasn't passed what we thought it was
				if(tailBase >= fh1) return t;
			}

			//Return value
			return t;
		}
	}

	inline std::size_t details::ThreadData::queueSize() const {
		return backTail.load(std::memory_order_acquire) - frontTail.load(std::memory_order_acquire);
	}

	struct details::YieldOp {
		std::function<bool()> predicate;
		Task task;
		bool await_ready() {
			return predicate();
		}
		void await_suspend(std::coroutine_handle<details::Promise> h) {
			//Get the task and mark it as yielded
			task = Task {h};
			task.promise().status = Status::Yielded;

			//Store ourselves in the yield list
			task.promise().pool.lock()->threads[task.promise().threadIdx].yields.push_back(*this);
		}
		void await_resume() {
			//Mark the task as executing again
			task.promise().status = Status::Executing;
		}
	};

	inline details::ThreadData::ThreadData() {}

	inline details::ThreadData::ThreadData(details::ThreadData&& o)
	  // clang-format off
	  : thread(std::exchange(o.thread, {})), yields(std::exchange(o.yields, {})), pool(std::exchange(o.pool, {})),
	  	curSteal(o.curSteal), myIndex(o.myIndex), ringbuf(std::exchange(o.ringbuf, {})), frontHead(o.frontHead.load(std::memory_order_acquire)), 
		frontTail(o.frontTail.load(std::memory_order_acquire)), backHead(o.backHead.load(std::memory_order_acquire)), backTail(o.backTail.load(std::memory_order_acquire)) {
		// clang-format on
		o.curSteal = 0;
		o.myIndex = 0;
		o.frontHead.store(0);
		o.frontTail.store(0);
		o.backHead.store(0);
		o.backTail.store(0);
	}

	inline details::ThreadData& details::ThreadData::operator=(details::ThreadData&& o) {
		if(this != &o) {
			thread = std::exchange(o.thread, {});
			yields = std::exchange(o.yields, {});
			pool = std::exchange(o.pool, {});
			ringbuf = std::exchange(o.ringbuf, {});
			curSteal = o.curSteal;
			o.curSteal = 0;
			myIndex = o.myIndex;
			o.myIndex = 0;
			frontHead.store(o.frontHead.load(std::memory_order_acquire));
			o.frontHead.store(0);
			frontTail.store(o.frontTail.load(std::memory_order_acquire));
			o.frontTail.store(0);
			backHead.store(o.backHead.load(std::memory_order_acquire));
			o.backHead.store(0);
			backTail.store(o.backTail.load(std::memory_order_acquire));
			o.backTail.store(0);
		}
		return *this;
	}

	inline details::YieldOp yieldUntilTrue(std::function<bool()> predicate) {
		details::YieldOp yld;
		yld.predicate = predicate;
		return yld;
	}

	template<typename Rep, typename Period>
	inline details::YieldOp yieldFor(const std::chrono::duration<Rep, Period>& duration) {
		details::YieldOp yld;
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		yld.predicate = [now, duration]() { return std::chrono::steady_clock::now() - now >= duration; };
		return yld;
	}

	inline details::YieldOp yieldUntil(std::chrono::steady_clock::time_point time) {
		details::YieldOp yld;
		if(time <= std::chrono::steady_clock::now()) throw std::logic_error("Cannot yield until a time in the past!");
		yld.predicate = [time]() { return std::chrono::steady_clock::now() >= time; };
		return yld;
	}

	template<typename T>
	inline details::YieldOp yieldUntilComplete(Future<T> future) {
		details::YieldOp yld;
		yld.predicate = [future]() { auto s = future.checkStatus(); return s == Status::Complete || s == Status::Failed; };
		return yld;
	}

	template<typename T>
	inline details::YieldOp yieldUntilComplete(MultiFuture<T> future) {
		details::YieldOp yld;
		yld.predicate = [future]() { auto s = future.checkStatus(); return s == Status::Complete || s == Status::Failed; };
		return yld;
	}

	template<typename T>
	void Future<T>::await() {
		Status s = checkStatus();
		while(s != Status::Complete && s != Status::Failed) {
			std::this_thread::yield();
			s = checkStatus();
		}
	}

	template<typename T>
	Status Future<T>::checkStatus() const noexcept {
		return task.promise().status;
	}

	template<typename T>
	std::enable_if_t<!std::is_void_v<T>, T&> Future<T>::operator*() {
		if(checkStatus() != Status::Complete) await();
		details::ValuePromise<T>& vp = static_cast<details::ValuePromise<T>&>(task.promise());
		if(vp.exception) std::rethrow_exception(vp.exception);
		return vp.val.value();
	}

	template<typename T>
	std::enable_if_t<!std::is_void_v<T>, const T&> Future<T>::operator*() const {
		if(checkStatus() != Status::Complete) await();
		details::ValuePromise<T>& vp = static_cast<details::ValuePromise<T>&>(task.promise());
		if(vp.exception) std::rethrow_exception(vp.exception);
		return vp.val.value();
	}

	template<typename T>
	std::enable_if_t<!std::is_void_v<T>, T*> Future<T>::operator->() {
		if(checkStatus() != Status::Complete) await();
		details::ValuePromise<T>& vp = static_cast<details::ValuePromise<T>&>(task.promise());
		if(vp.exception) std::rethrow_exception(vp.exception);
		return vp.val.value();
	}

	template<typename T>
	void MultiFuture<T>::await() {
		Status s = checkStatus();
		while(s != Status::Complete && s != Status::Failed) {
			std::this_thread::yield();
			s = checkStatus();
		}
	}

	template<typename T>
	std::enable_if_t<!std::is_void_v<T>, std::vector<T>> MultiFuture<T>::results() {
		if(checkStatus() == Status::Failed) throw std::runtime_error("Cannot get results; at least one task has failed!");
		if(checkStatus() != Status::Complete) await();
		std::vector<T> res;
		for(Future<T>& f : futures) {
			res.push_back(*f);
		}
		return res;
	}

	template<typename T>
	Status MultiFuture<T>::checkStatus() const noexcept {
		Status s = Status::Pending;
		bool fail = false;
		bool allDone = true;
		for(Future<T>& f : futures) {
			if(f.checkStatus() == Status::Pending) allDone = false;
			if(f.checkStatus() == Status::Scheduled) {
				allDone = false;
				s = Status::Scheduled;
			}
			if(f.checkStatus() == Status::Executing) {
				allDone = false;
				s = Status::Executing;
			}
			if(f.checkStatus() == Status::Failed) {
				fail = false;
			}
		}
		if(allDone) s = (fail ? Status::Failed : Status::Complete);
		return s;
	}

	inline void worker(std::stop_token stop, std::shared_ptr<Pool> p, std::size_t idx) {
		//Get data
		details::ThreadData& data = p->threads[idx];

		//Loop
		while(!stop.stop_requested()) {
			//Check the yield list
			for(auto it = data.yields.begin(); it != data.yields.end();) {
				if(it->predicate()) {
					it->task.resume();
					it = data.yields.erase(it);
				} else
					++it;
			}

			//Check the regular task queue
			if(data.queueSize() > 0) {
				//We have to try/catch here in case a steal happened right before here and the queue has become empty
				try {
					//Fetch the next task and run it
					Task t = data.pop();
					t.resume();
				} catch(...) {}
			} else {
				//Steal a task from somebody else's queue
				std::shared_ptr<Pool> p = data.pool.lock();
				while(true) {
					//Select a victim
					while(p->threads[data.curSteal].queueSize() <= 0) {
						++data.curSteal;
						if(data.curSteal >= p->threads.size()) data.curSteal = 0;
						if(data.curSteal == data.myIndex) ++data.curSteal;
					}

					//Commence thievery
					try {
						Task t = p->threads[data.curSteal].pop();
						t.resume();
						break;
					} catch(...) {}
				}
			}
		}
	}

	template<typename... Args>
	struct details::ArgsHolder {
		std::tuple<std::decay_t<Args>...> args;
	};

	template<typename F, typename Arg1, typename... Args, typename R = std::invoke_result_t<F&&, Arg1, Args&&...>>
		requires(!std::is_void_v<Arg1>)
	auto corowrap(std::weak_ptr<Pool> p, F&& f, Args&&... baseArgs) {
		//Set up argument holder
		std::shared_ptr<details::ArgsHolder<Arg1, Args...>> args = std::make_shared<details::ArgsHolder<Arg1, Args...>>();
		args->args = std::tuple<std::decay_t<Arg1>, std::decay_t<Args>...>();
		std::get<Args...>(args->args) = std::move(baseArgs...);

		//Set up first argument setter
		const auto setArg1 = [args](std::any val) {
			Arg1 a1 = std::any_cast<Arg1>(val);
			std::get<Arg1>(args->args) = std::move(a1);
		};

		//Actual function wrapping

		//Is this a coroutine (of a recognized type) already?
		if constexpr(std::is_base_of_v<R, Task>) {
			details::Promise* dp = nullptr;
			const auto wrap = [args, fn = std::forward<F>(f)](details::Promise** dp) {
				//Store promise data pointer and immediately suspend
				//This is so we can safely use the pointer above
				details::Promise* promise = *dp;
				co_await std::suspend_always {};

				//Create and start the real task function
				//Since Task::Promise has suspend_always for initial_suspend this won't run until an explicit resume() call is made and thus the args should be bound
				R inner = std::apply(fn, std::move(args->args));
				inner.promise().pool = promise->pool;
				inner.promise().status = Status::Executing;
				inner.promise().threadIdx = promise->threadIdx;
				inner.resume();

				//Await and return logic
				if constexpr(std::is_same_v<R, VoidTask>) {
					co_await inner;
					co_return;
				} else {
					co_return co_await inner;
				}
			};

			//Start task and update promise data
			Task wrapped = wrap(&dp);
			dp = &wrapped.promise();
			wrapped.resume();
			wrapped.promise().arg1Set = setArg1;
			wrapped.promise().pool = p;
			wrapped.promise().status = Status::Pending;
			return std::make_pair<R, decltype(setArg1)>(wrap(), std::move(setArg1));
		} else {
			//Void or not?
			if constexpr(std::is_void_v<R>) {
				const auto wrap = [args, fn = std::forward<F>(f)]() -> VoidTask {
					//Run the function
					std::apply(fn, std::move(args->args));
					co_return;
				};
				return std::make_pair<VoidTask, decltype(setArg1)>(wrap(), std::move(setArg1));
			} else {
				const auto wrap = [args, fn = std::forward<F>(f)]() -> ValueTask<R> {
					//Run the function
					co_return std::apply(fn, std::move(args->args));
				};
				return std::make_pair<ValueTask<R>, decltype(setArg1)>(wrap(), std::move(setArg1));
			}
		}
	}

	template<typename F, typename Arg1 = void, typename... Args, typename R = std::invoke_result_t<F&&, Args&&...>>
		requires std::is_void_v<Arg1>
	auto corowrap(std::weak_ptr<Pool> p, F&& f, Args&&... baseArgs) {
		//Set up argument holder
		std::shared_ptr<details::ArgsHolder<Args...>> args = std::make_shared<details::ArgsHolder<Args...>>();
		args->args = std::tuple<std::decay_t<Args>...>(baseArgs...);

		//Make a fake first argument setter
		const auto fakeSetArg1 = [args](std::any) {
			throw std::runtime_error("no");
		};

		//Actual function wrapping

		//Is this a coroutine (of a recognized type) already?
		if constexpr(std::is_base_of_v<R, Task>) {
			details::Promise* dp = nullptr;
			const auto wrap = [args, fn = std::forward<F>(f)](details::Promise** dp) {
				//Store promise data pointer and immediately suspend
				//This is so we can safely use the pointer above
				details::Promise* promise = *dp;
				co_await std::suspend_always {};

				//Create and start the real task function
				//Since Task::Promise has suspend_always for initial_suspend this won't run until an explicit resume() call is made and thus the args should be bound
				R inner = std::apply(fn, std::move(args->args));
				inner.promise().pool = promise->pool;
				inner.promise().status = Status::Executing;
				inner.promise().threadIdx = promise->threadIdx;
				inner.resume();

				//Await and return logic
				if constexpr(std::is_same_v<R, VoidTask>) {
					co_await inner;
					co_return;
				} else {
					co_return co_await inner;
				}
			};

			//Start task and update promise data
			Task wrapped = wrap(&dp);
			dp = &wrapped.promise();
			wrapped.resume();
			wrapped.promise().arg1Set = fakeSetArg1;
			wrapped.promise().pool = p;
			wrapped.promise().status = Status::Pending;
			return std::make_pair<R, decltype(fakeSetArg1)>(wrap(), std::move(fakeSetArg1));
		} else {
			//Void or not?
			if constexpr(std::is_void_v<R>) {
				const auto wrap = [args, fn = std::forward<F>(f)]() -> VoidTask {
					//Run the function
					std::apply(fn, std::move(args->args));
					co_return;
				};
				return std::make_pair<VoidTask, decltype(fakeSetArg1)>(wrap(), std::move(fakeSetArg1));
			} else {
				const auto wrap = [args, fn = std::forward<F>(f)]() -> ValueTask<R> {
					//Run the function
					co_return std::apply(fn, std::move(args->args));
				};
				return std::make_pair<ValueTask<R>, decltype(fakeSetArg1)>(wrap(), std::move(fakeSetArg1));
			}
		}
	}

	inline std::size_t Pool::getThreadCount() const noexcept {
		return threads.size();
	}

	inline void Pool::waitIdle() const noexcept {
		while(true) {
			bool exit = true;
			for(const details::ThreadData& td : threads) {
				if(td.queueSize() >= 0) {
					exit = false;
					break;
				}
			}
			if(exit)
				break;
			else
				std::this_thread::yield();
		}
	}

	inline Pool::Pool(std::size_t threadCount) {
		//Safety check
		if(totalThreads + threadCount > std::thread::hardware_concurrency()) throw std::out_of_range("Total number of threads used by pools would exceed hardware concurrency limit!");
		totalThreads += threadCount;

		//Spawn threads
		for(std::size_t i = 0; i < threadCount; i++) {
			//Setup data
			details::ThreadData& td = threads.emplace_back();
			td.pool = weak_from_this();
			td.myIndex = i;
			td.frontHead.store(0);
			td.frontTail.store(0);
			td.backHead.store(0);
			td.backTail.store(0);
			td.thread = std::jthread(worker, shared_from_this(), i);
		}
	}

	inline Pool::~Pool() {
		//Wait for idle
		waitIdle();

		//Stop workers
		for(details::ThreadData& td : threads) {
			td.thread.request_stop();
			td.thread.join();
		}

		//Decrement worker thread count
		totalThreads -= threads.size();
	}
}