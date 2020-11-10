#include "common_basic_phong_lighting.hlsl"

cbuffer VSConstantBuffer : register(b0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Projection;
}

VS_OUTPUT main_vs(
      float3 Position  : POSITION
    , float3 Normal    : NORMAL
    , float3 Tangent   : TANGENT
    , float2 Tex       : TEXCOORD0)
{
    VS_OUTPUT output  = (VS_OUTPUT)0;
    output.WorldPos   = (float3)mul(World, float4(Position, 1.0));
    output.Tex        = Tex;
    output.Position   = float4(output.WorldPos, 1.0);
    output.Position   = mul(View, output.Position);
    output.Position   = mul(Projection, output.Position);

    float3x3 World3x3 = (float3x3)World;
    // TODO: read "One last thing": https://learnopengl.com/Lighting/Basic-Lighting
    // also: https://stackoverflow.com/questions/13654401/why-transforming-normals-with-the-transpose-of-the-inverse-of-the-modelview-matr
    output.Normal     = normalize(mul(World3x3, normalize(Normal)));
    output.Tangent    = normalize(mul(World3x3, normalize(Tangent)));
    output.Binormal   = normalize(cross(output.Normal, output.Tangent));
    return output;
}
