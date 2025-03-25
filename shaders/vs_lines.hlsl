#include "common_lines.hlsl"

cbuffer VSConstantBuffer : register(b0)
{
    float4x4 Wold;
    float4x4 View;
    float4x4 Projection;
}

VS_OUTPUT main_vs(
      float3 Position : POSITION
    , float3 Color    : COLOR)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Position  = mul(Wold, float4(Position, 1.0));
    output.Position  = mul(View, output.Position);
    output.Position  = mul(Projection, output.Position);
    output.Color     = Color;
    return output;
}
