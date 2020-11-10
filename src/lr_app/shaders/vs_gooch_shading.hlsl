struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float3 WorldPos : POSITION;
};

cbuffer VSConstantBuffer : register(b0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Projection;
}

VS_OUTPUT main_vs(float3 Position : POSITION, float3 Normal : NORMAL)
{
    float3x3 World3x3 = (float3x3)World;

    VS_OUTPUT output  = (VS_OUTPUT)0;
    output.WorldPos   = (float3)mul(World, float4(Position, 1.0));
    output.Normal     = normalize(mul(World3x3, normalize(Normal)));
    output.Position   = float4(output.WorldPos, 1.0);
    output.Position   = mul(View, output.Position);
    output.Position   = mul(Projection, output.Position);
    return output;
}
