#include "ColorSpace.hlsli"

// ToneMapRenderer.h의 FToneMapConstants와 바이트 단위로 같아야 한다.
cbuffer ToneMapConstants : register(b0)
{
    float Exposure;     // 기본 1.0 (FToneMapRenderer::SetExposure로 조절)
    uint  Mode;         // 0 = ACES + sRGB 인코딩(Lit), 1 = sRGB 인코딩만(Unlit), 2 = 그대로 출력(WorldNormal·Heatmap 등 디버그)
    float2 ToneMapPadding;
};

#define TONEMAP_MODE_ACES        0
#define TONEMAP_MODE_ENCODE_ONLY 1
#define TONEMAP_MODE_PASSTHROUGH 2

Texture2D InputTexture : register(t0);

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
};

// Krzysztof Narkowicz의 ACES Filmic 근사. 입력과 출력 모두 선형 값이다.
float3 ACESFitted(float3 color)
{
    const float3x3 ACESInputMat =
    {
        { 0.59719, 0.35458, 0.04823 },
        { 0.07600, 0.87577, 0.04823 },
        { 0.02840, 0.13383, 0.83777 }
    };
    const float3x3 ACESOutputMat =
    {
        { 1.60475, -0.53108, -0.07367 },
        { -0.10208, 1.10813, -0.00605 },
        { -0.00327, -0.07276, 1.07602 }
    };
    float3 v = mul(ACESInputMat, color);

    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    float3 c = a / b;
    return saturate(mul(ACESOutputMat, c));
}

PSInput mainVS(uint VertexID : SV_VertexID)
{
    PSInput Output;
    float2 UV = float2((VertexID << 1) & 2, VertexID & 2); // (0,0) (2,0) (0,2)
    Output.Position = float4(UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    Output.UV = UV;
    return Output;
}

float4 mainPS(PSInput Input) : SV_Target
{
    const float3 SceneColor = max(InputTexture.Load(int3(Input.Position.xy, 0)).rgb, 0.0f);

    // 디버그 뷰는 셰이더가 쓴 값을 그대로 보여준다.
    if (Mode == TONEMAP_MODE_PASSTHROUGH)
    {
        return float4(saturate(SceneColor), 1.0f);
    }

    // Unlit은 텍스처 원본 색을 보여야 하므로 곡선 없이 인코딩만 한다.
    const float3 Mapped = (Mode == TONEMAP_MODE_ENCODE_ONLY)
        ? saturate(SceneColor)
        : ACESFitted(SceneColor * Exposure);

    // 씬은 선형으로 쌓였으므로 화면에 내보내기 전에 한 번만 sRGB로 인코딩한다.
    // ImGui가 뷰포트 텍스처를 알파 블렌딩으로 그리므로 알파는 1로 고정한다.
    return float4(LinearToSRGB(Mapped), 1.0f);
}

