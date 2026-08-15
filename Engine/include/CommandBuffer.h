#pragma once

#include <vector>
#include <functional>
#include "world.h"

// Worldへの操作を遅延実行するためのバッファ
class CommandBuffer
{
	std::vector<std::function<void(ECS::World&)>> commands;

public:
	// コマンド（ラムダ式など）を積む
	void Push(std::function<void(ECS::World&)> cmd) {
		commands.push_back(cmd);
	}

	// 溜まったコマンドをすべて実行する
	void Execute(ECS::World& world);
};