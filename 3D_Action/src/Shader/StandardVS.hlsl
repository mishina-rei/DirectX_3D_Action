// StandardVS.hlsl
//#include "Common.hlsli"

cbuffer CameraCB : register(b0)
{
    matrix viewProjection;
    matrix world;
    //float3 lightDir;
    //float padding;
};

//cbuffer ModelCB : register(b1)
//{
//    matrix world;
//};
//
//
struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
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
    float4 worldPos = mul(world, float4(input.pos, 1.0f));
    output.pos = mul(viewProjection, worldPos);
    output.normal = normalize(mul((float3x3) world, input.normal));
    output.uv = input.uv;
    
    // 行列を掛けずに、そのままの位置で出力する（動かなくてOK）
    //output.pos = float4(input.pos * 0.3f + float3(0.0f, 0.0f, 0.5f), 1.0f);
    //output.normal = input.normal;
    //output.uv = input.uv;
    return output;
}