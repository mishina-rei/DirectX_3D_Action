// StandardPS.hlsl
#include "Common.hlsli"

Texture2D MainTex : register(t0);
SamplerState MainSampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = MainTex.Sample(MainSampler, input.uv);
    
    float3 normal = normalize(input.normal);
    float3 lightVec = normalize(-lightDir);
    float diffuse = saturate(dot(normal, lightVec));
    float ambient = 0.2f;

    float3 finalColor = texColor.rgb * (diffuse + ambient);
    return float4(finalColor, texColor.a);
}