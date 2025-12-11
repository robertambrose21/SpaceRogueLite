// Tile composition vertex shader
// Renders tiles using per-instance data for position and atlas UV coordinates

cbuffer ProjectionUBO : register(b0, space1) {
    float4x4 projection;
};

struct VSInput {
    // Per-vertex (unit quad corners)
    float2 localPos : TEXCOORD0;    // 0-1 normalized quad position
    float2 localUV : TEXCOORD1;     // 0-1 normalized UV

    // Per-instance
    float2 tilePos : TEXCOORD2;     // World position (pixels)
    float4 tileUV : TEXCOORD3;      // Atlas UV (u_min, v_min, u_max, v_max)
};

struct VSOutput {
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
};

static const float TILE_SIZE = 64.0;

VSOutput main(VSInput input) {
    VSOutput output;

    // Scale local position to tile size and offset by instance position
    float2 worldPos = input.tilePos + input.localPos * TILE_SIZE;
    output.position = mul(projection, float4(worldPos, 0.0, 1.0));

    // Interpolate UV within the tile's atlas region
    output.texCoord = lerp(input.tileUV.xy, input.tileUV.zw, input.localUV);

    return output;
}
