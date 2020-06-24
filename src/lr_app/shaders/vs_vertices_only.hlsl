
cbuffer VSConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
};

VS_OUTPUT main_vs(float3 Position : POSITION)
{
    float4 p = float4(Position, 1.0);

    VS_OUTPUT output;
    output.Position   = mul(p, World);
    output.Position   = mul(output.Position, View);
    output.Position   = mul(output.Position, Projection);
    // output.Position = p;
    return output;
}
