struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
    float3 WorldPos : POSITION;
};

cbuffer VSConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

VS_OUTPUT main_vs(float4 Position : POSITION, float3 Normal : NORMAL)
{
    float3x3 World3x3 = (float3x3)World;

    VS_OUTPUT output  = (VS_OUTPUT)0;
    output.WorldPos   = (float3)mul(Position, World);
    output.Normal     = normalize(mul(normalize(Normal), World3x3));
    output.Position   = mul(Position, World);
    output.Position   = mul(output.Position, View);
    output.Position   = mul(output.Position, Projection);
    return output;
}
