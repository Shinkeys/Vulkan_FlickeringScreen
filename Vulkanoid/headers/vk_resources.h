#pragma once

#include <iostream>
#include <deque>
#include <functional>

#include "types/vk_types.h"


class ResourceManager
{
private:
	std::deque<std::function<void()>> _deletionQueue;
public:

	void PushFunction(const std::function<void()>&& func)
	{
		_deletionQueue.push_back(func);
	}

	void FlushQueue()
	{
		for (const auto& it : _deletionQueue)
		{
			(it)();

			_deletionQueue.pop_front();
		}
	}
};