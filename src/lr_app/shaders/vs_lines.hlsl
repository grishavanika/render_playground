#include "common_ps_lines.hlsl"

cbuffer VSConstantBuffer : register(b0)
{
    matrix Wold;
    matrix View;
    matrix Projection;
}

VS_OUTPUT main_vs(
      float4 Position : POSITION
    , float3 Color    : COLOR)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Position  = mul(Position, Wold);
    output.Position  = mul(output.Position, View);
    output.Position  = mul(output.Position, Projection);
    output.Color     = Color;
    return output;
}
