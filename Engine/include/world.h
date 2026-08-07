// world.h

#pragma once

#pragma once
#include <map>
#include <set>
#include <tuple>
#include "Archetype.h"
#include "SparsePool.h"
#include "IDGenerator.h"

namespace ECS 
{

    class World 
    {
    private:

        struct EntityRecord 
        {
            std::shared_ptr<Archetype> archetype;
            size_t index;
        };

        EntityID nextEntityId = 0;

        // アーキタイプ管理
        // Signature(vector<int>) をキーにする
        std::map<Signature, std::shared_ptr<Archetype>> archetypeMap;

        // EntityIDからarchetypeとそのインデックスへのマッピング
        std::unordered_map<EntityID, EntityRecord> entityIndex;

        // 空アーキタイプ
        std::shared_ptr<Archetype> emptyArchetype;

        // スパースデータ管理
        std::unordered_map<ComponentTypeID, std::shared_ptr<ISparsePool>> sparsePools;

        // アーキタイプに格納するコンポーネントを記録
        std::set<ComponentTypeID> structuralTypes;

        IDGenerator entityIdGen;

    public:

        World();

        // アーキタイプに格納するコンポーネントを登録
        template <typename T>
        void RegisterStructural() {
            structuralTypes.insert(ComponentRegistry::GetID<T>());
        }

        // エンティティを作成
        EntityID CreateEntity();

        // エンティティを削除
        void DeleteEntity(EntityID id);

        // コンポーネント追加
        template <typename T>
        void AddComponent(EntityID id, T data) 
        {
            ComponentTypeID typeId = ComponentRegistry::GetID<T>();
            // 構造的コンポーネントか？
            if (structuralTypes.count(typeId)) {
                AddToArchetype<T>(id, data, typeId);
            }
            else {
                AddToSparse<T>(id, data, typeId);
            }
        }

        // コンポーネント追加
        template <typename... Args>
        void AddComponents(EntityID id, Args... args)
        {
            EntityRecord& record = entityIndex[id];
            auto oldArch = record.archetype;

            // 新しいシグネチャを作成
            Signature newSig = oldArch->signature;

            // ラムダ式を定義（1要素分の処理）
            // アーキタイプコンポーネントはSignatureに記録する
            // スパースセットコンポーネントはそのまま追加する
            auto AddSingle = [&](auto& arg) {
                using T = std::remove_reference_t<decltype(arg)>; // 型Tを抽出

                ComponentTypeID typeId = ComponentRegistry::GetID<T>();
                if (structuralTypes.count(typeId)) {
                    // 既に追加されていたらreturn 
                    for (auto t : newSig) if (t == typeId) return;

                    AddToSignature(newSig, typeId);
                }
                else {
                    AddToSparse<T>(id, arg, typeId);
                }
            };

            // 畳み込み式でラムダを呼び出す
            (AddSingle(args), ...); 

            // 新アーキタイプ取得or作成
            auto newArch = GetOrCreateArchetype(newSig);

            // データ移行
            MigrateEntity(id, record, oldArch, newArch);

            // ラムダ式を定義（1要素分の処理）
            // アーキタイプコンポーネントは新しいアーキタイプにデータをセットする
            // スパースセットコンポーネントは処理しない
            auto SetData = [&](auto& arg) {
                using T = std::remove_reference_t<decltype(arg)>; // 型Tを抽出

                ComponentTypeID typeId = ComponentRegistry::GetID<T>();

                // アーキタイプに格納しないコンポーネントは飛ばす
                if (!structuralTypes.count(typeId))return;

                // 新しいコンポーネント用の配列がなければセット
                if (newArch->columns.find(typeId) == newArch->columns.end()) {
                    newArch->columns[typeId] = std::make_shared<ComponentArray<T>>();
                }
                auto col = std::static_pointer_cast<ComponentArray<T>>(newArch->columns[typeId]);

                // データ格納
                if (col->data.size() <= record.index) col->data.resize(record.index + 1);
                col->data[record.index] = arg;
            };

            (SetData(args), ...);
        }

        // コンポーネント削除
        template <typename T>
        void RemoveComponent(EntityID id) 
        {
            ComponentTypeID typeId = ComponentRegistry::GetID<T>();

            if (structuralTypes.count(typeId)) {
                RemoveFromArchetype<T>(id, typeId);
            }
            else {
                GetSparsePool<T>(typeId)->Remove(id);
            }
        }

        // コンポーネントを持っているか確認
        template <typename T>
        bool HasComponent(EntityID id) 
        {
            // コンポーネントのIDを取得
            ComponentTypeID typeId = ComponentRegistry::GetID<T>();

            // アーキタイプに入れるコンポーネントか確認
            if (structuralTypes.count(typeId)) 
            {
                if (entityIndex.find(id) == entityIndex.end()) return false;
                EntityRecord& r = entityIndex[id];
                return r.archetype->columns.find(typeId) != r.archetype->columns.end();
            }
            else {
                if (sparsePools.find(typeId) == sparsePools.end()) return false;
                // ISparsePoolからSparsePool<T>へキャストして確認
                auto pool = std::static_pointer_cast<SparsePool<T>>(sparsePools[typeId]);
                return pool->data.find(id) != pool->data.end();
            }
        }

        // コンポーネント取得
        template <typename T>
        T& GetComponent(EntityID id) 
        {
            // コンポーネントのIDを取得
            ComponentTypeID typeId = ComponentRegistry::GetID<T>();

            // アーキタイプに入れるコンポーネントか確認
            if (structuralTypes.count(typeId)) 
            {
                // アーキタイプから取得
                EntityRecord& r = entityIndex[id];
                auto col = std::static_pointer_cast<ComponentArray<T>>(r.archetype->columns[typeId]);
                return col->data[r.index];
            }
            else {
                // スパースから取得
                return GetSparsePool<T>(typeId)->Get(id);
            }
        }

        // 複数のStructuralコンポーネントを持つエンティティを走査
        template <typename... Components, typename Func>
        void ForEach(Func func) 
        {
            // コンポーネントのIDを取得
            std::vector<ComponentTypeID> typeIds = { ComponentRegistry::GetID<Components>()... };

            // アーキタイプを走査
            for (auto& [sig, arch] : archetypeMap) 
            {
                bool hasAll = true;

                // アーキタイプが指定のコンポーネントを含んでいるか確認
                for (auto id : typeIds) 
                {
                    // １つでも含んでいなかったら処理を終わる
                    if (arch->columns.find(id) == arch->columns.end()) {
                        hasAll = false;
                        break;
                    }
                }
                if (!hasAll) continue;

                // 指定のコンポーネントの配列をダウンキャストしてタプルにまとめる
                auto arrays = std::make_tuple(
                    static_cast<ComponentArray<Components>*>(arch->columns[ComponentRegistry::GetID<Components>()].get())...
                );

                size_t count = arch->entities.size();
                for (size_t i = 0; i < count; ++i) {
                    func(arch->entities[i], std::get<ComponentArray<Components>*>(arrays)->data[i]...);
                }
            }
        }
        
        // 単一コンポーネントを走査（Structural/Sparse両対応）
        template <typename T, typename Func>
        void ForEachComponent(Func func) {
            ComponentTypeID typeId = ComponentRegistry::GetID<T>();

            if (structuralTypes.count(typeId)) {
                // Structuralコンポーネントの場合
                for (auto& [sig, arch] : archetypeMap) 
                {
                    if (arch->columns.find(typeId) == arch->columns.end()) continue;

                    auto* compArray = static_cast<ComponentArray<T>*>(arch->columns[typeId].get());
                    size_t count = arch->entities.size();
                    for (size_t i = 0; i < count; ++i) {
                        func(arch->entities[i], compArray->data[i]);
                    }
                }
            }
            else {
                // Sparseコンポーネントの場合
                auto pool = GetSparsePool<T>(typeId);
                for (auto& pair : pool->data) {
                    func(pair.first, pair.second);
                }
            }
        }

    private:

        // アーキタイプに追加
        template <typename T>
        void AddToArchetype(EntityID id, T data, ComponentTypeID typeId) 
        {
            EntityRecord& record = entityIndex[id];
            auto oldArch = record.archetype;

            // 新しいシグネチャを作成
            Signature newSig = oldArch->signature;
            // 既に追加されていたらreturn 
            for (auto t : newSig) if (t == typeId) return;

            AddToSignature(newSig, typeId);

            // 新アーキタイプ取得or作成
            auto newArch = GetOrCreateArchetype(newSig);

            // データ移行
            MigrateEntity(id, record, oldArch, newArch);

            // 新しいコンポーネント用の配列がなければセット
            if (newArch->columns.find(typeId) == newArch->columns.end()) {
                newArch->columns[typeId] = std::make_shared<ComponentArray<T>>();
            }
            auto col = std::static_pointer_cast<ComponentArray<T>>(newArch->columns[typeId]);

            // データ格納
            if (col->data.size() <= record.index) col->data.resize(record.index + 1);
            col->data[record.index] = data;
        }

        // アーキタイプから削除（構造的コンポーネントの削除）
        template <typename T>
        void RemoveFromArchetype(EntityID id, ComponentTypeID typeId) 
        {
            EntityRecord& record = entityIndex[id];
            auto oldArch = record.archetype;

            // 署名から削除
            Signature newSig = oldArch->signature;
            RemoveFromSignature(newSig, typeId);

            // 新しいアーキタイプへ移動
            auto newArch = GetOrCreateArchetype(newSig);
            MigrateEntity(id, record, oldArch, newArch);
        }
        // SignatureからArchetypeを取得、なければArchetypeを作成
        std::shared_ptr<Archetype> GetOrCreateArchetype(const Signature& sig);

        // Entity移行
        void MigrateEntity(EntityID id, EntityRecord& record,std::shared_ptr<Archetype> oldArch,std::shared_ptr<Archetype> newArch);

        // スパースに追加
        template <typename T>
        void AddToSparse(EntityID id, T data, ComponentTypeID typeId) {
            GetSparsePool<T>(typeId)->Set(id, data);
        }

        template <typename T>
        std::shared_ptr<SparsePool<T>> GetSparsePool(ComponentTypeID typeId) 
        {
            if (sparsePools.find(typeId) == sparsePools.end()) {
                sparsePools[typeId] = std::make_shared<SparsePool<T>>();
            }
            return std::static_pointer_cast<SparsePool<T>>(sparsePools[typeId]);
        }
    };  
}
