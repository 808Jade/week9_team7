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

    // 고휘도에서 자연스럽게 채도가 빠지며 하얗게 번지는 정통 ACES 톤매핑
    float3 Mapped = ACESFitted(HDR);

    // SceneColor는 UNORM이므로 화면에 표시할 sRGB 근사값으로 변환한다.
    float3 DisplayColor = pow(saturate(Mapped), 1.0f / 2.2f);
    return float4(DisplayColor, 1.0f);
}
