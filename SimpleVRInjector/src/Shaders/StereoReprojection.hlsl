Texture2D g_Color : register(t0);
Texture2D<float> g_Depth : register(t1);
SamplerState g_Sampler : register(s0);

cbuffer Params : register(b0)
{
    float g_Separation;   // Eye separation factor
    float g_Convergence;  // Screen plane distance
    float g_EyeSign;      // -1 for Left, +1 for Right
    float g_Padding;
};

struct VS_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

// Simple Full-Screen Quad Vertex Shader
PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output;
    output.Pos = input.Pos;
    output.Tex = input.Tex;
    return output;
}

// Z-Buffer Reprojection Pixel Shader
float4 PS(PS_INPUT input) : SV_Target
{
    // Sample depth at the current UV
    float z = g_Depth.SampleLevel(g_Sampler, input.Tex, 0);
    
    // Linearize depth if needed (assuming standard non-linear Z for now)
    // For MVP, we use raw Z to approximate shift.
    // Near objects (low Z or high Z depending on buffer) shift more.
    
    // Simple Parallax: Shift X coordinate based on Depth
    // If Z is close (0.0 or 1.0 depending on convention), shift is max.
    // Let's assume 0.0 is near, 1.0 is far.
    // Shift = Separation * (1.0 - z) * EyeSign
    
    float shift = g_Separation * (1.0f - z) * g_EyeSign;
    
    float2 newUV = input.Tex + float2(shift, 0);
    
    // Clamp to avoid border artifacts
    if(newUV.x < 0 || newUV.x > 1) return float4(0,0,0,1);
    
    // Sample Color at displaced UV
    float4 color = g_Color.Sample(g_Sampler, newUV);
    
    return color;
}
