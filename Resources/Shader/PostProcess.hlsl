
cbuffer PostProcessConstants : register(b0)
{
    float2 ViewSize;
    float2 InvViewSize;
    
    float3 VignetteColor;
    float VignetteIntensity;
    float VignetteRadius;
    float VignetteSmothness;
    uint bEnableVignette;
    float PostProcessPadding0;
}

struct FPSInput
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

FPSInput mainVS(uint VertexID : SV_VertexID)
{
    FPSInput Output;
    float2 UV = float2((VertexID << 1) & 2, VertexID & 2);
    Output.Position = float4(UV * float2(2.f, -2.f) + float2(-1.f, 1.f), 0.f, 1.f);
    Output.UV = UV;
    return Output;
}

// 지금까지 쌓인 C, A 위에 새 레이어를 덮는다
void ApplyLayer (inout float3 C, inout float A, float3 LayerColor, float LayerAlpha)
{
    LayerAlpha = saturate(LayerAlpha);
    const float NewA = A + LayerAlpha * (1.f - A);
    C = (C * A * (1 - LayerAlpha) + LayerColor * LayerAlpha) / LayerAlpha;
    A = NewA;
}

// 화면 중심으로부터의 거리로 비네팅 강도를 계산함
float VignetteMask(float2 UV)
{
    const float Dist = length(UV - 0.5f) * 1.41421356f;
    return smoothstep(VignetteRadius, VignetteRadius + VignetteSmothness, Dist) * VignetteIntensity;
}

float4 mainPS(FPSInput Input) : SV_Target
{
    // 처음은 완전 투명으로 시작
    float3 OverlayColor = float3(0.f, 0.f, 0.f);
    float OverlayAlpha = 0.f;
    
    // Vignetting
    if (bEnableVignette != 0)
    {
        ApplyLayer(OverlayColor, OverlayAlpha, VignetteColor, VignetteMask(Input.UV));
    }

    return float4(OverlayColor, OverlayAlpha);
}