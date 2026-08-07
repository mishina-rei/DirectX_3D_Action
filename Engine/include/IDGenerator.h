#pragma once

#include <queue>
#include <cstdint>

class IDGenerator
{
public:
	using ID = uint32_t;

	IDGenerator() : m_nextId(0) {}

	// ID生成
	ID Generate()
	{
		if (!m_freeIds.empty())
		{
			ID id = m_freeIds.front();
			m_freeIds.pop();
			return id;
		}
		return m_nextId++;
	}

	// ID解放（再利用）
	void Release(ID id)
	{
		m_freeIds.push(id);
	}

	// 全リセット
	void Reset()
	{
		m_nextId = 0;
		std::queue<ID> empty;
		std::swap(m_freeIds, empty);
	}

private:
	ID m_nextId;
	std::queue<ID> m_freeIds;
};