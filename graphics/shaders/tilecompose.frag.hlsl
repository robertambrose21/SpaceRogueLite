// Tile composition fragment shader
// Samples from tile atlas texture

Texture2D<float4> AtlasTexture : register(t0, space2);
SamplerState AtlasSampler : register(s0, space2);

struct PSInput {
    float2 texCoord : TEXCOORD0;
};

struct PSOutput {
    float4 color : SV_Target0;
};

PSOutput main(PSInput input) {
    PSOutput output;
    output.color = AtlasTexture.Sample(AtlasSampler, input.texCoord);
    return output;
}
