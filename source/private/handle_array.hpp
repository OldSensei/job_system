#pragma once
#include <array>
#include <queue>
#include <cstdint>
#include <iostream>
#include <mutex>

template<typename T>
struct HandleArrayItemTraits
{
	static constexpr bool isStoredId = false;
};

template<typename UnderlyingType, typename HandleType, std::uint32_t size>
class HandleArray
{
public:
	HandleArray() :
		m_handleMutex{},
		m_storage{},
		m_freeHandles{}
	{
		for (std::uint32_t index = 0; index < size; ++index)
			m_freeHandles.push(static_cast<HandleType>(index)); //to many allocation ((((
	}

	~HandleArray()
	{
	}

	const UnderlyingType& get( const HandleType& handle ) const
	{
		return *std::launder(reinterpret_cast<const UnderlyingType*>(&m_storage[handle]));
	}

	UnderlyingType& get(const HandleType& handle)
	{
		return *std::launder(reinterpret_cast<UnderlyingType*>(&m_storage[handle]));
	}

	template<typename ... Args>
	HandleType emplace(Args&&... args)
	{
		auto handle = getFreeHandle();
		if constexpr ( HandleArrayItemTraits<UnderlyingType>::isStoredId )
		{
			new (&m_storage[handle]) UnderlyingType{ handle, std::forward<Args>(args)... };
		}
		else
		{
			new (&m_storage[handle]) UnderlyingType{ std::forward<Args>(args)... };
		}

		return handle;
	}


	void free(const HandleType& handle)
	{
		auto& obj = *std::launder(reinterpret_cast<UnderlyingType*>(&m_storage[handle]));
		obj.~UnderlyingType();
		putHandleToFree(handle);
	}

private:
	HandleType getFreeHandle()
	{
		std::lock_guard guard(m_handleMutex);
		auto handle = m_freeHandles.front();
		m_freeHandles.pop();
		return handle;
	}

	void putHandleToFree(const HandleType& handle)
	{
		std::lock_guard guard(m_handleMutex);
		m_freeHandles.push( handle );
	}

private:
	using StorageType = std::aligned_storage_t<sizeof(UnderlyingType), alignof(UnderlyingType)>;

	std::array< StorageType, size > m_storage;
	std::mutex m_handleMutex;
	std::queue<HandleType> m_freeHandles;
};
