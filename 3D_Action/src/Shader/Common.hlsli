// Common.hlsli
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