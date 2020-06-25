
cbuffer VSConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
};

VS_OUTPUT main_vs(float3 Position : POSITION, float3 Normal : NORMAL)
{
    float4 p = float4(Position, 1.0);

    VS_OUTPUT output;
    
    output.Position   = mul(p, World);
    output.Position   = mul(output.Position, View);
    output.Position   = mul(output.Position, Projection);
    // output.Position = p;

    float3x3 World3x3 = (float3x3)World;
    output.Normal     = normalize(mul(normalize(Normal), World3x3));
    return output;
}
