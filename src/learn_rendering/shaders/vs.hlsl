cbuffer ConstantBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
}

struct VS_OUTPUT
{
	float4 Pos : SV_POSITION;
	float3 Norm : NORMAL;
};

VS_OUTPUT main_vs(float4 Pos : POSITION, float3 Norm : NORMAL)
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.Pos = mul(Pos, World);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);
	output.Norm = normalize(mul(normalize(Norm), (float3x3)World));
	return output;
}
