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
#include <coroutine>
#include <cstddef>
#include <exception>
#include <memory>
#include <ranges>
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
		Scheduled,///<The task has been scheduled for execution but has not begun
		Cancelled,///<Execution of the task was cancelled
		Executing,///<The task is currently executing
		Yielded,  ///<The task has yielded temporarily
		Failed,	  ///<The task completed with an exception
		Complete  ///<The task completed successfully
	};

	///@cond
	namespace details {
		struct Promise;
		struct VoidPromise;
		struct ValuePromise;
		struct TaskGenerator;
		struct YieldOp;
		struct ThreadData;
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
		Task(const Task& other) : h(other.h) {}

		/**
		 * @brief Copy assignment
		 */
		Task& operator=(const Task& other) {
			if(this != &other) {
				h = other.h;
			}
			return *this;
		}

		/**
		 * @brief Move construction
		 */
		Task(Task&& other) : h(std::exchange(other.h, {})) {}

		/**
		 * @brief Move assignment
		 */
		Task& operator=(Task&& other) noexcept {
			if(this != &other) {
				//Destroy coroutine state before swap
				if(h) h.destroy();
				h = std::exchange(other.h, {});
			}
			return *this;
		}

		~Task() noexcept {
			if(h) h.destroy();
		}

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
	 */
	class ValueTask : public Task {
	  public:
		using promise_type = details::ValuePromise;

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
		///@cond
		using value_type = T;
		///@endcond

		/**
		 * @brief Block until execution had completed
		 *
		 * @throws std::runtime_error If the future is cancelled during this operation
		 */
		void await();

		/**
		 * @brief Get the status of a future
		 *
		 * @returns The future's current status
		 */
		Status checkStatus() const noexcept;

		/**
		 * @brief Cancel the future if it has not yet been executed
		 *
		 * This function does nothing if the future has already begun execution, been cancelled, or completed.
		 */
		void cancel() noexcept;

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
		 * @throws std::logic_error If the future has been completed or cancelled
		 * @throws std::bad_weak_ptr If the pool to which this future belongs no longer exists
		 */
		template<typename F, typename... ExArgs, typename R = std::invoke_result_t<F&&, T&&, ExArgs&&...>>
			requires std::invocable<F&&, T&&, ExArgs&&...>
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
		 * @throws std::logic_error If the future has been completed or cancelled
		 * @throws std::bad_weak_ptr If the pool to which this future belongs no longer exists
		 */
		template<typename F, typename... ExArgs>
			requires std::invocable<F&&, T&&, ExArgs&&...> && std::is_void_v<std::invoke_result_t<F&&, T&&, ExArgs&&...>>
		void thenDetached(F func, ExArgs... exargs);

		/**
		 * @brief Schedule a tracked batch job based on a container for execution after this future
		 *
		 * @tparam Rn Input range type
		 * @tparam F Function type
		 * @tparam ExArgs Extra rguments to the function
		 * @tparam R Function return type
		 *
		 * @param src The source range to iterate over
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 *
		 * @throws std::logic_error If the future has been completed or cancelled
		 * @throws std::bad_weak_ptr If the pool to which this future belongs no longer exists
		 */
		template<std::ranges::input_range Rn, typename F, typename... ExArgs, typename R = std::invoke_result_t<F&&, T&&, Rn&&, ExArgs&&...>>
			requires std::invocable<F&&, T&&, Rn&&, ExArgs&&...>
		[[nodiscard]] MultiFuture<R> thenBatch(Rn&& src, F func, ExArgs... exargs);

		/**
		 * @brief Schedule a batch job based on a container for execution after this future with no result
		 *
		 * @tparam Rn Input range type
		 * @tparam F Function type
		 * @tparam ExArgs Extra rguments to the function
		 *
		 * @param src The source range to iterate over
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 *
		 * @throws std::logic_error If the future has been completed or cancelled
		 * @throws std::bad_weak_ptr If the pool to which this future belongs no longer exists
		 */
		template<std::ranges::input_range Rn, typename F, typename... ExArgs>
			requires std::invocable<F&&, T&&, Rn&&, ExArgs&&...> && std::is_void_v<std::invoke_result_t<F&&, T&&, Rn&&, ExArgs&&...>>
		void thenBatchDetached(Rn&& src, F func, ExArgs... exargs);

		/**
		 * @brief Obtain the task result, blocking if not complete
		 *
		 * This function does not exist in the @c void specialization of this type
		 *
		 * @return The result of the task
		 *
		 * @throws std::runtime_error If the future is cancelled during this operation
		 * @throws The exception thrown by the task if failed
		 */
		template<typename U = T>
		std::enable_if_t<!std::is_void_v<U>, U&> operator*();

		/**
		 * @brief Obtain the task result, blocking if not complete
		 *
		 * This function does not exist in the @c void specialization of this type
		 *
		 * @return The result of the task
		 *
		 * @throws std::runtime_error If the future is cancelled during this operation
		 * @throws The exception thrown by the task if failed
		 */
		template<typename U = T>
		std::enable_if_t<!std::is_void_v<U>, const U&> operator*() const;

		/**
		 * @brief Obtain the task result, blocking if not complete
		 *
		 * This function does not exist in the @c void specialization of this type
		 *
		 * @return The result of the task
		 *
		 * @throws std::runtime_error If the future is cancelled during this operation
		 * @throws The exception thrown by the task if failed
		 */
		template<typename U = T>
		std::enable_if_t<!std::is_void_v<U>, U*> operator->();

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
		explicit MultiFuture(std::shared_ptr<Future<T>>, ...);

		/**
		 * @brief Get the number of collected futures
		 *
		 * @returns Future count
		 */
		std::size_t size() const noexcept;

		/**
		 * @brief Block until all futures have completed execution
		 *
		 * @throws std::runtime_error If any future is cancelled during this operation
		 */
		void await();

		/**
		 * @brief Get the overall status of the collection
		 *
		 * @details The status progresses as follows: starting at Scheduled, once a future starts executing the status is set to Executing, then Complete once all futures have completed.\n The status will be set to Cancelled if a future is cancelled and likewise for Failed. The Yielding status is never returned.
		 *
		 * @returns The overall status
		 */
		Status checkStatus() const noexcept;

		/**
		 * @brief Cancel all futures that have not yet been executed
		 *
		 * This function does nothing to futures that have already begun execution, been cancelled, or completed.
		 */
		void cancel() noexcept;

		/**
		 * @brief Get the pool that the futures exist on
		 *
		 * @returns The pool if it still exists, or an empty pointer otherwise
		 */
		std::weak_ptr<Pool> getPool() noexcept;

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
		 * @throws std::logic_error If the future has been completed or cancelled
		 * @throws std::bad_weak_ptr If the pool to which the futures belong no longer exists
		 */
		template<typename F, typename... ExArgs, typename R = std::invoke_result_t<F&&, std::vector<T>&&, ExArgs&&...>>
			requires std::invocable<F&&, std::vector<T>&&, ExArgs&&...>
		[[nodiscard]] std::shared_ptr<Future<R>> then(F func, ExArgs... exargs);

		/**
		 * @brief Schedule a tracked task for execution after these futures with no result
		 *
		 * @tparam F Function type
		 * @tparam ExArgs Extra arguments to the function
		 *
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 *
		 * @throws std::logic_error If the future has been completed or cancelled
		 * @throws std::bad_weak_ptr If the pool to which the futures belong no longer exists
		 */
		template<typename F, typename... ExArgs>
			requires std::invocable<F&&, std::vector<T>&&, ExArgs&&...> && std::is_void_v<std::invoke_result_t<F&&, std::vector<T>&&, ExArgs&&...>>
		void thenDetached(F func, ExArgs... exargs);

		/**
		 * @brief Schedule a tracked batch job based on a container for execution after these futures
		 *
		 * @tparam Rn Input range type
		 * @tparam F Function type
		 * @tparam ExArgs Extra rguments to the function
		 * @tparam R Function return type
		 *
		 * @param src The source range to iterate over
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 *
		 * @throws std::logic_error If the future has been completed or cancelled
		 * @throws std::bad_weak_ptr If the pool to which the futures belong no longer exists
		 */
		template<std::ranges::input_range Rn, typename F, typename... ExArgs, typename R = std::invoke_result_t<F&&, std::vector<T>&&, Rn&&, ExArgs&&...>>
			requires std::invocable<F&&, std::vector<T>&&, Rn&&, ExArgs&&...>
		[[nodiscard]] MultiFuture<R> thenBatch(Rn&& src, F func, ExArgs... exargs);

		/**
		 * @brief Schedule a batch job based on a container for execution after these futures with no result
		 *
		 * @tparam Rn Input range type
		 * @tparam F Function type
		 * @tparam ExArgs Extra rguments to the function
		 *
		 * @param src The source range to iterate over
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 *
		 * @throws std::logic_error If the future has been completed or cancelled
		 * @throws std::bad_weak_ptr If the pool to which the futures belong no longer exists
		 */
		template<std::ranges::input_range Rn, typename F, typename... ExArgs>
			requires std::invocable<F&&, std::vector<T>&&, Rn&&, ExArgs&&...> && std::is_void_v<std::invoke_result_t<F&&, std::vector<T>&&, Rn&&, ExArgs&&...>>
		void thenBatchDetached(Rn&& src, F func, ExArgs... exargs);

		/**
		 * @brief Get the results of the futures, blocking if not complete
		 *
		 * @returns A list of results corresponding to the order of futures as placed in the constructor
		 *
		 * @throws std::runtime_error If any future is cancelled during this operation
		 * @throws std::runtime_error If any of the futures failed
		 */
		std::vector<T> results();

	  private:
		std::vector<std::shared_ptr<Future<T>>> futures;
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

		//Move only
		///@cond
		Pool(const Pool&) = delete;
		Pool& operator=(const Pool&) = delete;
		Pool(Pool&&) = default;
		Pool& operator=(Pool&&) = default;
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
		[[nodiscard]] std::shared_ptr<Future<R>> submit(F func, Args... args);

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
		 * @tparam Rn Input range type
		 * @tparam F Function type
		 * @tparam ExArgs Extra rguments to the function
		 * @tparam R Function return type
		 *
		 * @param src The source range to iterate over
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 */
		template<std::ranges::input_range Rn, typename F, typename... ExArgs, typename R = std::invoke_result_t<F&&, Rn&&, ExArgs&&...>>
			requires std::invocable<F&&, Rn&&, ExArgs&&...>
		[[nodiscard]] MultiFuture<R> batch(Rn&& src, F func, ExArgs... exargs);

		/**
		 * @brief Submit a batch job based on a container into the pool with no result
		 *
		 * @tparam Rn Input range type
		 * @tparam F Function type
		 * @tparam ExArgs Extra rguments to the function
		 *
		 * @param src The source range to iterate over
		 * @param func The function to invoke
		 * @param exargs Extra arguments to pass to the function
		 */
		template<std::ranges::input_range Rn, typename F, typename... ExArgs>
			requires std::invocable<F&&, Rn&&, ExArgs&&...> && std::is_void_v<std::invoke_result_t<F&&, Rn&&, ExArgs&&...>>
		void batchDetached(Rn&& src, F func, ExArgs... exargs);

		/**
		 * @brief Get the number of worker threads managed the pool
		 *
		 * @returns Pool thread count
		 */
		std::size_t getThreadCount();

		/**
		 * @brief Wait until there are no more tasks in the queue (tasks submitted during this call will continue to block it)
		 */
		void waitIdle();

		/**
		 * @brief Cancel all currently scheduled tasks
		 */
		void cancel();

	  private:
		Pool(std::size_t threadCount);

		static std::size_t totalThreads;

		std::vector<details::ThreadData> threads;
		std::vector<int> threadsByLeastTasks;
		void resortThreads();
	};

	/**
	 * @brief Get the pool the current thread belongs to (or nothing if this isn't a worker thread)
	 *
	 * @returns An optional containing a pool pointer if the current thread is a worker, or nothing otherwise
	 */
	std::optional<std::shared_ptr<Pool>> getCurrentThreadPool();

	/**
	 * @brief Suspend execution of your task and allow other tasks to run for a certain period of time
	 *
	 * @note As the use of this function makes your function a coroutine, it must explicitly return a Task and use @c co_return to be valid
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
	 * @note As the use of this function makes your function a coroutine, it must explicitly return a Task and use @c co_return to be valid
	 *
	 * @param time The point in time until which to yield. It is not guaranteed that execution will resume exactly at the specified time point
	 *
	 * @return An awaitable object; you must use @c co_await on this result to yield correctly
	 *
	 * @throws std::logic_error If the specified time point is in the past
	 */
	template<typename Rep, typename Period>
	[[nodiscard]] details::YieldOp yieldUntil(std::chrono::steady_clock::time_point time);

	/**
	 * @brief Suspend execution of your task and allow other tasks to run until a certain condition is met
	 *
	 * @note As the use of this function makes your function a coroutine, it must explicitly return a Task and use @c co_return to be valid
	 *
	 * @param predicate A function that will evaluate the condition to determine if execution should resume. It should not have side effects.
	 *
	 * @return An awaitable object; you must use @c co_await on this result to yield correctly
	 */
	[[nodiscard]] details::YieldOp yieldUntilTrue(std::function<bool()> predicate);

	/**
	 * @brief Suspend execution of your task and allow other tasks to run until a future resolves
	 *
	 * @note As the use of this function makes your function a coroutine, it must explicitly return a Task and use @c co_return to be valid
	 * @warning If the submitted future is cancelled during the yield, the task state will be destroyed and execution will not resume.\n Manually-allocated memory will not be freed. Consider using smart pointers to avoid this.
	 *
	 * @param future The future of which to yield until completion. It is not guaranteed that execution will resume exactly when the future becomes complete.
	 *
	 * @return An awaitable object; you must use @c co_await on this result to yield correctly
	 *
	 * @throws std::logic_error If the specified future has already been completed or cancelled
	 */
	template<typename T>
	[[nodiscard]] details::YieldOp yieldUntilComplete(Future<T> future);

	/**
	 * @brief Suspend execution of your task and allow other tasks to run until futures resolve
	 *
	 * @note As the use of this function makes your function a coroutine, it must explicitly return a Task and use @c co_return to be valid
	 * @warning If any of the submitted futures are cancelled during the yield, the task state will be destroyed and execution will not resume.\n Manually-allocated memory (for example, raw @c new or @c malloc) will not be freed. Consider using smart pointers to avoid this.
	 *
	 * @param futures The futures of which to yield until completion. It is not guaranteed that execution will resume exactly when the futures become complete.
	 *
	 * @return An awaitable object; you must use @c co_await on this result to yield correctly
	 *
	 * @throws std::logic_error If the specified futures have already been completed or cancelled
	 */
	template<typename T>
	[[nodiscard]] details::YieldOp yieldUntilComplete(MultiFuture<T> futures);
}

//=============== IMPLEMENTATION ===============

namespace exathread {
	namespace details {
		struct Promise {
			std::any val;					  //The stored result value
			std::exception_ptr exception;	  //The stored result exception (if one is thrown)
			Status status;					  //The status of the task
			std::weak_ptr<Pool> pool;		  //The pool of execution
			details::TaskGenerator* generator;//The generator for the task

			void unhandled_exception() noexcept {
				exception = std::current_exception();
			}

			std::suspend_always initial_suspend() noexcept {
				status = Status::Scheduled;
				return {};
			}

			std::suspend_always final_suspend() noexcept {
				status = exception ? Status::Failed : Status::Complete;

				//Schedule continuations if we're sure this all worked
				if(!exception && val.has_value() && !pool.expired() && generator && generator->next) {
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

		struct TaskGenerator {
		  public:
			TaskGenerator* next = nullptr;

			TaskGenerator(std::function<Task()> genfunc, std::weak_ptr<Pool> p) {
				generator = [this, genfunc, p]() {
					Task task = genfunc();
					task.promise().generator = this;
					task.promise().pool = p;
					return task;
				};
			}

			Task generate() {
				return generator();
			}

		  private:
			std::function<Task()> generator;
		};

		struct ValuePromise : public Promise {
			template<typename T>
				requires(!std::is_void_v<T>)
			void return_value(T value) noexcept(std::is_nothrow_move_constructible_v<T>) {
				val = std::move(value);
			}

			Task get_return_object() noexcept override {
				return ValueTask {std::coroutine_handle<details::Promise>::from_promise(*this)};
			}

			void continuation() override {
				//TODO
			}
		};

		struct VoidPromise : public Promise {
			void return_void() noexcept {}

			Task get_return_object() noexcept override {
				return VoidTask {std::coroutine_handle<details::Promise>::from_promise(*this)};
			}

			void continuation() override {
				//TODO
			}
		};

		struct YieldOp {
			std::function<bool()> predicate;
			Task task;
			bool await_ready() {
				return predicate();
			}
			void await_suspend(std::coroutine_handle<details::Promise> h) {
				//Get the task and mark it as yielded
				task = Task {h};
				task.promise().status = Status::Yielded;
			}
			void await_resume() {
				//Mark the task as executing again
				task.promise().status = Status::Executing;
			}
		};

		struct ThreadData {
			std::jthread thread;
			std::stop_source stop;
			std::vector<std::shared_ptr<YieldOp>> yields;
		};
	}

	inline void worker(std::stop_token stop, details::ThreadData& data) {
		while(!stop.stop_requested()) {
			//First check the yield list
			for(auto yptr : data.yields) {
				if(yptr->predicate()) yptr->task.resume();
			}
		}
	}
}