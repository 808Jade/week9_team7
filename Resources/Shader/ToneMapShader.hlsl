#include "ColorSpace.hlsli"

// C++의 FToneMapConstants와 바이트 단위로 같아야 한다. (16바이트)
cbuffer ToneMapConstants : register(b0)
{
    float Gamma; // 디스플레이 감마. 기본 2.2
    uint bEnableGamma; // Show Flag. 0이면 감마 보정 없이 선형 값을 그대로 출력
    float2 ToneMapPadding;
};

Texture2D<float4> LightingHDR : register(t0);

struct FPSInput
{
    float4 Position : SV_POSITION;
};

FPSInput mainVS(uint VertexID : SV_VertexID)
{
    FPSInput Output;
    float2 UV = float2((VertexID << 1) & 2, VertexID & 2);
    Output.Position = float4(UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return Output;
}

// Stephen Hill & Krzysztof Narkowicz의 ACES Fitted 공식 (AP0/AP1 색공간 변환 및 Crosstalk 포함)
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

float4 mainPS(FPSInput Input) : SV_Target
{
    int2 Pixel = int2(Input.Position.xy);
    float3 HDR = max(LightingHDR.Load(int3(Pixel, 0)).rgb, 0.0f);

    // 고휘도에서 자연스럽게 채도가 빠지며 하얗게 번지는 ACES 톤매핑 (선형 -> 선형 [0,1])
    float3 Mapped = ACESFitted(HDR);

    // Gamma Correction: 선형 -> 모니터 감마 공간. 이후 패스(반투명/안개/텍스트)는 이 공간에 그대로 그린다.
    // 꺼져 있으면 선형 값이 그대로 나가서 화면이 어둡게 보인다. (비교용)
    float3 DisplayColor = (bEnableGamma != 0) ? LinearToGamma(Mapped, Gamma) : Mapped;
    return float4(DisplayColor, 1.0f);
}