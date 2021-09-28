//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

cbuffer SceneConstantBuffer : register(b0)
{
    float4 velocity;
    float4 offset;
    float4 color;
    float4x4 projection;
};

cbuffer RootConstants : register(b1)
{
    float4 size;
};

ByteAddressBuffer inputBuffer: register(t0);

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD0;
};

struct VSInput
{
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput result;

    result.position = mul(input.position + offset, projection);

    float intensity = saturate((4.0f - result.position.z) / 2.0f);
    result.color = float4(color.xyz * intensity, 1.0f);
    result.texcoord = input.texcoord;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float2 screenUV = input.position.xy / size.xy;
    uint2 uv = screenUV.xy * size.zw;
    uint offset = uv.y * size.z + uv.x;
    uint3 ucolor = inputBuffer.Load3(offset * 3 * 4);
    float4 color = float4(asfloat(ucolor.x), asfloat(ucolor.y), asfloat(ucolor.z), 1.f);
    return color * input.color;
}
