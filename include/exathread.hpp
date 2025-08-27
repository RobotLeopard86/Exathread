/**
 * @file exathread.hpp
 * @author RobotLeopard86
 * @version 1.0.0
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

#include <cstddef>
#include <thread>

///@brief The root namespace for all Exathread functionality
namespace exathread {
	/**
	 * @brief A task in a given pool which will eventually resolve to a result
	 */
	template<typename T = void>
	class Task {
	  public:
		using value_type = T;

		/**
		 * @brief The current state of a task
		 */
		enum class Status {
			Scheduled,
			Executing,
			Yielded,
			Complete
		};

		/**
		 * @brief Block until the task has completed execution
		 */
		void await();

		/**
		 * @brief Get the status of a task
		 *
		 * @returns The task's current status
		 */
		Status checkStatus() const noexcept;

		//Result accessors

		/**
		 * @brief Obtain the task result, blocking if not complete
		 *
		 * This function does not exist in the @c void specialization of this type
		 *
		 * @return The result of the task
		 */
		template<typename U = T>
		std::enable_if_t<!std::is_void_v<U>, U&> operator*();

		template<typename U = T>
		std::enable_if_t<!std::is_void_v<U>, const U&> operator*() const;

		template<typename U = T>
		std::enable_if_t<!std::is_void_v<U>, U*> operator->();

		//Move only
		///@cond
		Task(const Task&) = delete;
		Task& operator=(const Task&) = delete;
		Task(Task&&) = default;
		Task& operator=(Task&&) = default;
		///@endcond
	};

	/**
	 * @brief A group of threads to execute tasks
	 */
	class Pool {
	  public:
		/**
		 * @brief Create a new pool with a set amount of threads
		 *
		 * @param threadCount The number of threads to assign to the pool (half of the hardware concurrency by default)
		 *
		 * @throws std::out_of_range If the amount of threads consumed by all pools would exceed std::thread::hardware_concurrency if this pool were created
		 */
		explicit Pool(std::size_t threadCount = std::thread::hardware_concurrency() / 2);

		//Move only
		///@cond
		Pool(const Pool&) = delete;
		Pool& operator=(const Pool&) = delete;
		Pool(Pool&&) = default;
		Pool& operator=(Pool&&) = default;
		///@endcond

		/**
		 * @brief Submit a task into the pool, expecting a result
		 *
		 * @tparam F Function type
		 * @tparam Args Arguments to the function
		 * @tparam R Function return type
		 *
		 * @param func The function to invoke
		 * @param args Arguments to pass to the function;
		 */
		template<typename F, typename... Args, typename R = std::invoke_result_t<F&&, Args&&...>>
			requires std::invocable<F&&, Args&&...>
		Task<R> submit(F func, Args... args);

		/**
		 * @brief Submit a task into the pool with no result
		 *
		 * @tparam F Function type
		 * @tparam Args Arguments to the function
		 *
		 * @param func The function to invoke
		 * @param args Arguments to pass to the function;
		 */
		template<typename F, typename... Args>
			requires std::invocable<F&&, Args&&...> && std::is_void_v<std::invoke_result_t<F&&, Args&&...>>
		void submitDetached(F func, Args... args);
	};
}

//The implementation goes down here