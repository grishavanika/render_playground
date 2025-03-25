
cbuffer VSConstantBuffer : register(b0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Projection;
}

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
};

VS_OUTPUT main_vs(float3 Position : POSITION)
{
    VS_OUTPUT output;
    output.Position = mul(World, float4(Position, 1.0));
    output.Position = mul(View, output.Position);
    output.Position = mul(Projection, output.Position);
    return output;
}
