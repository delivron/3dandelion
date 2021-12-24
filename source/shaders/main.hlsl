struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

cbuffer camera : register(b0)
{
    float4x4 modelViewProjection;
}

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;
    result.position = mul(modelViewProjection, position);
    result.color = color;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
