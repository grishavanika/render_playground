
struct VS_OUTPUT
{
    float4 Position : SV_POSITION0;
    float2 Tex      : TEXCOORD0;
    float3 WorldPos : POSITION0;

    float3 Normal   : NORMAL0;
    float3 Tangent  : TANGENT0;
    float3 Binormal : BINORMAL0;
};
