// Textured quad vertex shader
// Transforms 2D positions using orthographic projection and passes UV coordinates

cbuffer TransformUBO : register(b0, space1) {
    float4x4 projection;
};

struct VSInput {
    float2 position : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
};

struct VSOutput {
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

VSOutput main(VSInput input) {
    VSOutput output;
    output.position = mul(projection, float4(input.position, 0.0, 1.0));
    output.texCoord = input.texCoord;
    return output;
}
