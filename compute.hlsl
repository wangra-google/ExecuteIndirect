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

#define threadBlockSize 128

struct SceneConstantBuffer
{
    float4 velocity;
    float4 offset;
    float4 color;
    float4x4 projection;
    float4 padding[9];
};

struct IndirectCommand
{
    uint2 cbvAddress;
    uint2 srvAddress;
    float4 rootConstants;
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int BaseVertexLocation;
    uint StartInstanceLocation;
};

cbuffer RootConstants : register(b0)
{
    float xOffset;        // Half the width of the triangles.
    float zOffset;        // The z offset for the triangle vertices.
    float cullOffset;    // The culling plane offset in homogenous space.
    float commandCount;    // The number of commands to be processed.
};

// not sure why this is not working?!?!
//cbuffer IndirectArgsConstants : register(b1)
//{
//    uint2 srvAddress[2];
//    float4 rootConstants[2];
//    uint indexCountPerInstance[2];
//    float padding[50];
//};

cbuffer IndirectArgsConstants : register(b1)
{
    uint2 srvAddress0;
    uint2 srvAddress1;
    float4 rootConstants0;
    float4 rootConstants1;
    uint indexCountPerInstance0;
    uint indexCountPerInstance1;
    float padding[50];
};

StructuredBuffer<SceneConstantBuffer> cbv                : register(t0);    // SRV: Wrapped constant buffers
StructuredBuffer<IndirectCommand> inputCommands            : register(t1);    // SRV: Indirect commands
AppendStructuredBuffer<IndirectCommand> outputCommands    : register(u0);    // UAV: Processed indirect commands

[numthreads(threadBlockSize, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    // Each thread of the CS operates on one of the indirect commands.
    uint index = (groupId.x * threadBlockSize) + groupIndex;

    // Don't attempt to access commands that don't exist if more threads are allocated
    // than commands.
    if (index < commandCount)
    {
        // Project the left and right bounds of the triangle into homogenous space.
        float4 left = float4(-xOffset, 0.0f, zOffset, 1.0f) + cbv[index].offset;
        left = mul(left, cbv[index].projection);
        left /= left.w;

        float4 right = float4(xOffset, 0.0f, zOffset, 1.0f) + cbv[index].offset;
        right = mul(right, cbv[index].projection);
        right /= right.w;

        IndirectCommand cmd; 
        cmd.BaseVertexLocation = 0;
        cmd.IndexCountPerInstance = 6; // TODO_RW: change this!!!
        cmd.InstanceCount = 1;
        cmd.StartIndexLocation = 0;
        cmd.StartInstanceLocation = 0;
        cmd.cbvAddress = inputCommands[index].cbvAddress;
        cmd.srvAddress = inputCommands[index].srvAddress;
        cmd.rootConstants = inputCommands[index].rootConstants;
        if (-cullOffset < right.x && left.x < cullOffset)
        {
            cmd.srvAddress = srvAddress0;
            cmd.rootConstants = rootConstants0;
        }
        else
        {
            cmd.srvAddress = srvAddress1;
            cmd.rootConstants = rootConstants1;
        }
        outputCommands.Append(cmd);
    }
}
