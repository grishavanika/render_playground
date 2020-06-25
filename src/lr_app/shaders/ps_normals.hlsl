struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 Normal : NORMAL;
};

float4 main_ps(VS_OUTPUT input) : SV_Target
{
    float3 n = normalize(input.Normal);
    float3 light_dir = float3(0.0, 0.0, -1.0);
    float3 object_colot = float3(1., 0., 0.);
    float3 color = saturate(dot(light_dir, n)) * object_colot + 0.1;
    return float4(color, 1.);
}
