// Textured quad fragment shader
// Samples texture at interpolated UV coordinates

Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

struct PSInput {
    float2 texCoord : TEXCOORD0;
};

struct PSOutput {
    float4 color : SV_Target0;
};

PSOutput main(PSInput input) {
    PSOutput output;
    output.color = Texture.Sample(Sampler, input.texCoord);
    return output;
}
