
#include "Engine_pch.h"

#include "CommandBuffer.h"
#include "ScriptSystem.h"

void CommandBuffer::Execute(ECS::World& world)
{
	int count = 0;	// 無限ループ防止用カウンタ
	while(true)
	{
		// コピーを作成してからクリアし、実行中に新たなコマンドが追加されても安全に動作するようにする
		std::vector<std::function<void(ECS::World&)>> copyCommands;
		copyCommands = std::move(commands);
		commands.clear();

		// 命令を実行
		for (auto& cmd : copyCommands)
		{
			cmd(world);
		}

		ScriptSystem::Init(&world);

		if (commands.empty() || 20 < count)
			return;

		count++;
	}
}
