#include "common_basic_phong_lighting.hlsl"

cbuffer VSConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

VS_OUTPUT main_vs(
      float4 Position  : POSITION
    , float3 Normal    : NORMAL
    , float3 Tangent   : TANGENT
    , float2 Tex       : TEXCOORD0)
{
    VS_OUTPUT output  = (VS_OUTPUT)0;
    output.WorldPos   = (float3)mul(Position, World);
    output.Tex        = Tex;
    output.Position   = mul(Position, World);
    output.Position   = mul(output.Position, View);
    output.Position   = mul(output.Position, Projection);

    float3x3 World3x3 = (float3x3)World;
    // TODO: read "One last thing": https://learnopengl.com/Lighting/Basic-Lighting
    // also: https://stackoverflow.com/questions/13654401/why-transforming-normals-with-the-transpose-of-the-inverse-of-the-modelview-matr
    output.Normal     = normalize(mul(normalize(Normal), World3x3));
    output.Tangent    = normalize(mul(normalize(Tangent), World3x3));
    output.Binormal   = normalize(cross(output.Normal, output.Tangent));
    return output;
}
