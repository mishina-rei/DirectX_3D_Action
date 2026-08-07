// ComponentArray.h

#pragma once

#include <vector>
#include <memory>

namespace ECS {

    // アーキタイプ用のコンポーネント配列
    // 型消去されたインターフェース
    class IComponentArray 
    {
    public:
        virtual ~IComponentArray() = default;
        virtual void MoveData(size_t srcIndex, IComponentArray* destArray, size_t destIndex) = 0;
        virtual void RemoveSwap(size_t index) = 0;
        virtual std::shared_ptr<IComponentArray> CreateNew() = 0;
    };

    template <typename T>
    class ComponentArray : public IComponentArray 
    {
    public:
        std::vector<T> data;

        // srcIndexに格納されたデータをdestArrayのdestIndexの場所に格納する
        void MoveData(size_t srcIndex, IComponentArray* destArray, size_t destIndex) override 
        {
            auto* dest = static_cast<ComponentArray<T>*>(destArray);
            if (dest->data.size() <= destIndex) {
                dest->data.resize(destIndex + 1);
            }
            dest->data[destIndex] = std::move(data[srcIndex]);
        }

        // 削除、末尾と入れ替えて削除
        void RemoveSwap(size_t index) override 
        {
            size_t lastIndex = data.size() - 1;
            if (index != lastIndex) {
                data[index] = std::move(data[lastIndex]);
            }
            data.pop_back();
        }

        std::shared_ptr<IComponentArray> CreateNew() override 
        {
            return std::make_shared<ComponentArray<T>>();
        }
    };
}