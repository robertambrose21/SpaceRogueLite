// Colored quad fragment shader
// Outputs color from uniform buffer

cbuffer ColorUBO : register(b0, space2) {
    float4 entityColor;
};

struct PSOutput {
    float4 color : SV_Target0;
};

PSOutput main() {
    PSOutput output;
    output.color = entityColor;
    return output;
}
