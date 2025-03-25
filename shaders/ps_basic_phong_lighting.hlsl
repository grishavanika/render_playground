#include "common_basic_phong_lighting.hlsl"

Texture2D TextureDiffuse   : register(t0);
Texture2D TextureNormal    : register(t1);
SamplerState SamplerLinear : register(s0);

struct PointLight
{
    float4 LightColor;
    float4 LightPosition; // World Space.
};

#define MAX_LIGHTS_COUNT 16

cbuffer PSConstantBuffer0 : register(b0)
{
    PointLight Lights[MAX_LIGHTS_COUNT];
    float4 ViewerPosition;
    // x = has texture
    // y = lights count
    float4 Parameters;
};

float4 main_ps(VS_OUTPUT input) : SV_Target
{
    float3 object_color;
    float3 normal;

    if (Parameters.x > 0)
    {
        object_color = (float3)TextureDiffuse.Sample(SamplerLinear, input.Tex);
        // object_color = float3(1.0, 1.0, 1.0);

        float3 tangent_normal = (float3)TextureNormal.Sample(SamplerLinear, input.Tex);
        // sRGB.
        tangent_normal = pow(abs(tangent_normal), 1/2.2);

        tangent_normal = tangent_normal * 2.0 - 1.0; // [0; 1] -> [-1; 1]
        float3x3 TBN = float3x3(
              normalize(input.Tangent)
            , normalize(input.Binormal)
            , normalize(input.Normal));

        // https://stackoverflow.com/questions/16555669/hlsl-normal-mapping-matrix-multiplication
        normal = mul(tangent_normal, TBN);
    }
    else
    {
        object_color = float3(1.0, 1.0, 1.0);
        normal = normalize(input.Normal);
    }

    // TODO: read more about Phong Model. Example:
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/phong-shader-BRDF
    float ambient_strength = 0.1;
    // Compared to what is in "Basic Lighting":
    // https://learnopengl.com/Lighting/Basic-Lighting
    // we don't multiply by LightColor so ambient
    // does not depend on the actual light source (?)
    float3 ambient = ambient_strength; // * LightColor.rgb;

    float3 d         = Lights[0].LightPosition.xyz - input.WorldPos;
    float3 light_dir = normalize(d);
    float  diff_k    = max(dot(normal, light_dir), 0.0);
    float3 diffuse   = diff_k * Lights[0].LightColor.xyz;
    
    // Note: learnopengl does not do this.
    diffuse = diffuse / length(d);

    float specular_strength = 5;
    float shininess         = 16;
    float3 view_dir         = normalize(ViewerPosition.xyz - input.WorldPos);
    float3 reflect_dir      = reflect(-light_dir, normal);
    float spec              = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
    float3 specular         = specular_strength * spec * Lights[0].LightColor.xyz;

    // Note: learnopengl does not do this.
    specular = specular / length(d);

    float3 final_color = object_color * (ambient + diffuse + specular);
    return float4(final_color, 1.0);
}
