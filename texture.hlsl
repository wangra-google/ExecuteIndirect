
cbuffer RootConstants : register(b0)
{
    uint width;       
    uint height;
};

Texture2D           g_texture : register(t0);
RWByteAddressBuffer outputBuffer: register(u0);

[numthreads(8, 8, 1)]
void CSMain(uint3 texelID: SV_DispatchThreadID)
{
    if ((texelID.x<width) && (texelID.y<height))
    {
        float4 color = g_texture.Load(texelID);
        uint3 ucol = uint3(asuint(color.x), asuint(color.y), asuint(color.z));
        uint offset = texelID.y * width + texelID.x;
        outputBuffer.Store3(offset * 3 * 4, ucol);
    }
}
