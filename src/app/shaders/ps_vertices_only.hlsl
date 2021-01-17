struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
};

float4 main_ps(VS_OUTPUT input) : SV_Target
{
    return float4(1.0, 0.0, 0.0, 1.0);
}
