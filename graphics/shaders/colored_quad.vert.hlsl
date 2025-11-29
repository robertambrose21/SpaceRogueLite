// Simple square vertex shader
// Transforms 2D positions using orthographic projection

cbuffer TransformUBO : register(b0, space1) {
    float4x4 projection;
};

struct VSInput {
    float2 position : TEXCOORD0;
};

struct VSOutput {
    float4 position : SV_Position;
};

VSOutput main(VSInput input) {
    VSOutput output;
    output.position = mul(projection, float4(input.position, 0.0, 1.0));
    return output;
}
