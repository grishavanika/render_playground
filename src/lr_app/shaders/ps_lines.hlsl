#include "common_lines.hlsl"

float4 main_ps(VS_OUTPUT input) : SV_Target
{
    return float4(input.Color, 1.0);
}
