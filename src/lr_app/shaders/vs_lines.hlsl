#include "common_ps_lines.hlsl"

cbuffer VSConstantBuffer : register(b0)
{
    matrix View;
    matrix Projection;
}

VS_OUTPUT main_vs(
      float4 WorldPos : POSITION
    , float3 Color    : COLOR)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.WorldPos  = mul(WorldPos, View);
    output.WorldPos  = mul(output.WorldPos, Projection);
    output.Color     = Color;
    return output;
}
