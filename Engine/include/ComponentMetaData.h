// ComponentMetaData.h
#pragma once
#include "world.h" 
#include <functional>
#include <string>


struct ComponentMetaData {
    std::string name;
    ECS::ComponentTypeID componentTypeId = 0;
	bool canRemove = true;  // 削除可能かどうかを示すフラグ
    std::function<bool(ECS::World*, ECS::EntityID)> hasComponent;   
	std::function<void(ECS::World*, ECS::EntityID)> removeComponent;    // コンポーネントが削除されたときのコールバック関数

    // UI関連 
	std::function<void(ECS::World*, ECS::EntityID)> drawInspector;      // インスペクター描画用のコールバック関数
	std::function<void(ECS::World*, ECS::EntityID)> addComponent;       // コンポーネント追加用のコールバック関数

};
