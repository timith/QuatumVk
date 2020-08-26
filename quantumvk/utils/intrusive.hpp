/* Copyright (c) 2017-2020 Hans-Kristian Arntzen
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <cstddef>
#include <utility>
#include <memory>
#include <atomic>
#include <type_traits>

namespace Util
{
	class SingleThreadCounter
	{
	public:
		inline void AddRef()
		{
			count++;
		}

		inline bool Release()
		{
			return --count == 0;
		}

	private:
		size_t count = 1;
	};

	class MultiThreadCounter
	{
	public:
		MultiThreadCounter()
		{
			count.store(1, std::memory_order_relaxed);
		}

		inline void AddRef()
		{
			count.fetch_add(1, std::memory_order_relaxed);
		}

		inline bool Release()
		{
			auto result = count.fetch_sub(1, std::memory_order_acq_rel);
			return result == 1;
		}

	private:
		std::atomic_size_t count;
	};

	template <typename T>
	class IntrusivePtr;

	//Enables an object to be RefCounted
	template <typename T, typename Deleter = std::default_delete<T>, typename ReferenceOps = SingleThreadCounter>
	class IntrusivePtrEnabled
	{
	public:
		using IntrusivePtrType = IntrusivePtr<T>;
		using EnabledBase = T;
		using EnabledDeleter = Deleter;
		using EnabledReferenceOp = ReferenceOps;

		void ReleaseReference()
		{
			if (reference_count.Release())
				Deleter()(static_cast<T*>(this));
		}

		void AddRef()
		{
			reference_count.AddRef();
		}

		IntrusivePtrEnabled() = default;

		IntrusivePtrEnabled(const IntrusivePtrEnabled&) = delete;

		void operator=(const IntrusivePtrEnabled&) = delete;

	protected:
		Util::IntrusivePtr<T> ReferenceFromThis();

	private:
		ReferenceOps reference_count;
	};

	//Intrusitve shared pointer
	template <typename T>
	class IntrusivePtr
	{
	public:
		template <typename U>
		friend class IntrusivePtr;

		IntrusivePtr() = default;

		explicit IntrusivePtr(T* handle)
			: data(handle)
		{
		}

		T& operator*()
		{
			return *data;
		}

		const T& operator*() const
		{
			return *data;
		}

		T* operator->()
		{
			return data;
		}

		const T* operator->() const
		{
			return data;
		}

		operator bool() { return data != nullptr; }
		operator bool() const { return data != nullptr; }


		bool operator==(const IntrusivePtr& other) const
		{
			return data == other.data;
		}

		bool operator!=(const IntrusivePtr& other) const
		{
			return data != other.data;
		}

		T* Get()
		{
			return data;
		}

		const T* Get() const
		{
			return data;
		}

		void Reset()
		{
			using ReferenceBase = IntrusivePtrEnabled<
				typename T::EnabledBase,
				typename T::EnabledDeleter,
				typename T::EnabledReferenceOp>;

			// Static up-cast here to avoid potential issues with multiple intrusive inheritance.
			// Also makes sure that the pointer type actually inherits from this type.
			if (data)
				static_cast<ReferenceBase*>(data)->ReleaseReference();
			data = nullptr;
		}

		template <typename U>
		IntrusivePtr& operator=(const IntrusivePtr<U>& other)
		{
			static_assert(std::is_base_of<T, U>::value,
				"Cannot safely assign downcasted intrusive pointers.");

			using ReferenceBase = IntrusivePtrEnabled<
				typename T::EnabledBase,
				typename T::EnabledDeleter,
				typename T::EnabledReferenceOp>;

			Reset();
			data = static_cast<T*>(other.data);

			// Static up-cast here to avoid potential issues with multiple intrusive inheritance.
			// Also makes sure that the pointer type actually inherits from this type.
			if (data)
				static_cast<ReferenceBase*>(data)->AddRef();
			return *this;
		}

		IntrusivePtr& operator=(const IntrusivePtr& other)
		{
			using ReferenceBase = IntrusivePtrEnabled<
				typename T::EnabledBase,
				typename T::EnabledDeleter,
				typename T::EnabledReferenceOp>;

			if (this != &other)
			{
				Reset();
				data = other.data;
				if (data)
					static_cast<ReferenceBase*>(data)->AddRef();
			}
			return *this;
		}

		template <typename U>
		IntrusivePtr(const IntrusivePtr<U>& other)
		{
			*this = other;
		}

		IntrusivePtr(const IntrusivePtr& other)
		{
			*this = other;
		}

		~IntrusivePtr()
		{
			Reset();
		}

		template <typename U>
		IntrusivePtr& operator=(IntrusivePtr<U>&& other) noexcept
		{
			Reset();
			data = other.data;
			other.data = nullptr;
			return *this;
		}

		IntrusivePtr& operator=(IntrusivePtr&& other) noexcept
		{
			if (this != &other)
			{
				Reset();
				data = other.data;
				other.data = nullptr;
			}
			return *this;
		}

		template <typename U>
		IntrusivePtr(IntrusivePtr<U>&& other) noexcept
		{
			*this = std::move(other);
		}

		template <typename U>
		IntrusivePtr(IntrusivePtr&& other) noexcept
		{
			*this = std::move(other);
		}

	private:
		T* data = nullptr;
	};

	template <typename T, typename Deleter, typename ReferenceOps>
	IntrusivePtr<T> IntrusivePtrEnabled<T, Deleter, ReferenceOps>::ReferenceFromThis()
	{
		AddRef();
		return IntrusivePtr<T>(static_cast<T*>(this));
	}

	template <typename Derived>
	using DerivedIntrusivePtrType = IntrusivePtr<Derived>;

	template <typename T, typename... P>
	DerivedIntrusivePtrType<T> MakeHandle(P&&... p)
	{
		return DerivedIntrusivePtrType<T>(new T(std::forward<P>(p)...));
	}

	template <typename Base, typename Derived, typename... P>
	typename Base::IntrusivePtrType MakeDerivedHandle(P&&... p)
	{
		return typename Base::IntrusivePtrType(new Derived(std::forward<P>(p)...));
	}

	template <typename T>
	using ThreadSafeIntrusivePtrEnabled = IntrusivePtrEnabled<T, std::default_delete<T>, MultiThreadCounter>;
}