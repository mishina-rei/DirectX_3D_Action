// StandardVS.hlsl
//#include "Common.hlsli"

#define MAX_BONES 256

cbuffer CameraCB : register(b0)
{
    matrix viewProjection;
    matrix world;
    matrix boneTransforms[MAX_BONES]; // ← これを追加！CPU側から行列配列を受け取る
};

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    int4 boneIDs : BLENDINDICES; // CPU側の BoneIDs[4] に対応
    float4 boneWeight : BLENDWEIGHT; // CPU側の BoneWeights[4] に対応
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

PSInput main(VSInput input)
{
    PSInput output;
    
    // ここからスキニング計算
    row_major matrix boneTransform = 0;
    
    // 4つのボーンの影響を重み(Weight)を掛けて足し合わせる
    for (int i = 0; i < 4; ++i)
    {
        // -1は「影響なし」なのでスキップ
        if (input.boneIDs[i] >= 0 && input.boneIDs[i] < MAX_BONES)
        {
            // ボーン行列 × ウェイト
            boneTransform += boneTransforms[input.boneIDs[i]] * input.boneWeight[i];
        }
    }
    
    // もしボーンの影響が一つもなければ、単位行列(そのまま)として扱う
    if (input.boneIDs[0] == -1)
    {
        boneTransform = matrix(1, 0, 0, 0,
                               0, 1, 0, 0,
                               0, 0, 1, 0,
                               0, 0, 0, 1);
    }
    
    //matrix boneTransform = boneTransforms[0];
    
    //if (boneTransform._11 == 0 && boneTransform._22 == 0)
    //{
    //    boneTransform = matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    //}
    
    // ボーンの計算で頂点を動かす (ローカル座標内での変形)
    float4 localPos = mul(float4(input.pos, 1.0f), boneTransform);
    //localPos.w = 1.0f;
    // それをワールド空間 -> ビュー・プロジェクション空間へ変換
    float4 worldPos = mul(localPos, world);
    //worldPos.w = 1.0f;
    output.pos = mul(worldPos, viewProjection);
    
    // Normalもボーンに合わせて回転させる(スケーリングの影響を消す処理は一旦省略)
    output.normal = mul(input.normal, (float3x3) boneTransform);
    output.normal = mul(output.normal, (float3x3) world);
    output.normal = normalize(output.normal);

    output.uv = input.uv;
    return output;
}