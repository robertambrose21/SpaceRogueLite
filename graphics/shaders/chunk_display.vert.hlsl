// Chunk display vertex shader
// Renders multiple chunks in a single instanced draw call using a texture array
// Each instance represents one chunk with its own world position and texture layer

cbuffer TransformUBO : register(b0, space1) {
    float4x4 viewProjection;
};

struct VSInput {
    // Per-vertex data (unit quad 0-1)
    float2 localPos : TEXCOORD0;
    float2 localUV : TEXCOORD1;
    // Per-instance data
    float2 chunkWorldPos : TEXCOORD2;   // World position of chunk's top-left corner
    float2 chunkSize : TEXCOORD3;       // Size in pixels (may be smaller for edge chunks)
    float layerIndex : TEXCOORD4;       // Texture array layer index
};

struct VSOutput {
    float4 position : SV_Position;
    float3 texCoord : TEXCOORD0;  // xy = UV, z = layer index
};

VSOutput main(VSInput input) {
    VSOutput output;

    // Scale unit quad by chunk size and offset by world position
    float2 worldPos = input.chunkWorldPos + input.localPos * input.chunkSize;
    output.position = mul(viewProjection, float4(worldPos, 0.0, 1.0));

    // Pass UV and layer index to fragment shader
    output.texCoord = float3(input.localUV, input.layerIndex);

    return output;
}
