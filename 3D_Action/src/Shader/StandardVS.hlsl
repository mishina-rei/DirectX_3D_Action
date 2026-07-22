// StandardVS.hlsl
#include "Common.hlsli"

PSInput main(VSInput input)
{
    PSInput output;
    float4 worldPos = mul(world, float4(input.pos, 1.0f));
    output.pos = mul(viewProjection, worldPos);
    output.normal = normalize(mul((float3x3) world, input.normal));
    output.uv = input.uv;
    return output;
}