// SpritePS.hlsl

Texture2D tex : register(t0);
SamplerState samp : register(s0);

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD;
    float4 Color : COLOR;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    // テクスチャからピクセルの色をサンプリング
    float4 texColor = tex.Sample(samp, input.Tex);
    
    // テクスチャの色に、C++から送られた色（透明度含む）を掛け合わせる
    return texColor * input.Color;
}