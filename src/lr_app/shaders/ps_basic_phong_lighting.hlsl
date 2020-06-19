#include "common_basic_phong_lighting.hlsl"

Texture2D TextureDiffuse   : register(t0);
SamplerState SamplerLinear : register(s0);

cbuffer PSConstantBuffer0 : register(b0)
{
    float4 LightColor;
    float4 LightPosition; // World Space.
    float4 ViewerPosition;
};

float4 main_ps(VS_OUTPUT input) : SV_Target
{
#if (0) // If diffuse texture present.
    float3 object_color = (float3)TextureDiffuse.Sample(SamplerLinear, input.Tex);
#else   // 
    float3 object_color = float3(1.0, 1.0, 1.0);
#endif

    // TODO: read more about Phong Model. Example:
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/phong-shader-BRDF
    float ambient_strength = 0.1;
    // Compared to what is in "Basic Lighting":
    // https://learnopengl.com/Lighting/Basic-Lighting
    // we don't multiply by LightColor so ambient
    // does not depend on the actual light source (?)
    float3 ambient = ambient_strength;// * (float3)LightColor;

    float3 norm      = input.Norm;
    float3 d         = (float3)LightPosition - input.WorldPos;
    float3 light_dir = normalize(d);
    float  diff_k    = max(dot(norm, light_dir), 0.0);
    float3 diffuse   = diff_k * (float3)LightColor;
    
    // Note: learnopengl does not do this.
    diffuse = diffuse / length(d);

    float specular_strength = 5;
    float shininess         = 16;
    float3 view_dir         = normalize((float3)ViewerPosition - input.WorldPos);
    float3 reflect_dir      = reflect(-light_dir, norm);
    float spec              = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
    float3 specular         = specular_strength * spec * (float3)LightColor; 

    // Note: learnopengl does not do this.
    specular = specular / length(d);

    float3 final_color = object_color * (ambient + diffuse + specular);
    return float4(final_color, 1);
}
