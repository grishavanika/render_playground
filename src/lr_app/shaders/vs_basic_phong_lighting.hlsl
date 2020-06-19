#include "common_basic_phong_lighting.hlsl"

cbuffer VSConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

VS_OUTPUT main_vs(
      float4 Pos  : POSITION
    , float3 Norm : NORMAL
    , float2 Tex  : TEXCOORD0)
{
    VS_OUTPUT output  = (VS_OUTPUT)0;
    output.WorldPos   = (float3)mul(Pos, World);
    output.Tex        = Tex;
    output.Pos        = mul(Pos, World);
    output.Pos        = mul(output.Pos, View);
    output.Pos        = mul(output.Pos, Projection);
    // TODO: read "One last thing": https://learnopengl.com/Lighting/Basic-Lighting
    // also: https://stackoverflow.com/questions/13654401/why-transforming-normals-with-the-transpose-of-the-inverse-of-the-modelview-matr
    output.Norm       = normalize(mul(normalize(Norm), (float3x3)World));
    return output;
}
