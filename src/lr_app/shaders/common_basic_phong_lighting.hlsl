
struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 Tex      : TEXCOORD0;
    float3 WorldPos : POSITION;

    float3 Normal   : NORMAL;
    float3 Tangent  : TANGENT;
    float3 Binormal : BINORMAL;
};
