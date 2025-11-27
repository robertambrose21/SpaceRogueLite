// Simple square fragment shader
// Outputs solid red color

struct PSOutput {
    float4 color : SV_Target0;
};

PSOutput main() {
    PSOutput output;
    output.color = float4(1.0, 0.0, 0.0, 1.0);
    return output;
}
