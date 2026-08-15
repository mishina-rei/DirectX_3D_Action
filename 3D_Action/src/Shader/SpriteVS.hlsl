// SpriteVS.hlsl

cbuffer SpriteCB : register(b0)
{
    matrix world;
    matrix viewProjection;
    
    // x: offsetX, y: offsetY, z: sizeX, w: sizeY
    float4 spriteParam;
    
    // x: uvPosX, y: uvPosY, z: uvScaleX, w: uvScaleY
    float4 uvParam;
    
    float4 color;
};

// C++側の VERTEX 構造体と完全に一致させる
struct VS_INPUT
{
    float3 Pos : POSITION;
    float2 Tex : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
    float4 Color : COLOR;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    // スケール(size)とピボット(offset)の適用
    // C++側で作成した 1.0 x 1.0 の板ポリゴンを、指定したサイズ・位置に変形します
    float3 localPos = input.Pos;
    localPos.x = (localPos.x * spriteParam.z) + spriteParam.x;
    localPos.y = (localPos.y * spriteParam.w) + spriteParam.y;

    // 行列変換
    float4 worldPos = mul(float4(localPos, 1.0f), world);
    output.Pos = mul(worldPos, viewProjection);

    // UVのスケールとスクロール位置の適用
    output.Tex = (input.Tex * uvParam.zw) + uvParam.xy;
    
    // スプライトの色・透明度
    output.Color = color;

    return output;
}