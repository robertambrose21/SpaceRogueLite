// Chunk display fragment shader
// Samples from a texture array using the layer index passed from vertex shader

Texture2DArray<float4> ChunkTextures : register(t0, space2);
SamplerState Sampler : register(s0, space2);

struct PSInput {
    float3 texCoord : TEXCOORD0;  // xy = UV, z = layer index
};

struct PSOutput {
    float4 color : SV_Target0;
};

PSOutput main(PSInput input) {
    PSOutput output;
    output.color = ChunkTextures.Sample(Sampler, input.texCoord);
    return output;
}
