
cbuffer PostProcessConstants : register(b0)
{
    float2 ViewSize;
    float2 InvViewSize;
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

float4 mainPS(FPSInput Input) : SV_Target
{
    float3 OverlayColor = float3(0.f, 0.f, 0.f);
    float OverlayAlpha = 0.f;

    return float4(OverlayColor, OverlayAlpha);
}