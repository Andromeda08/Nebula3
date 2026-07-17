#pragma once

struct GPUCameraData
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewInverse;
    float4x4 projInverse;
    float4   eye;
    float    nearPlane;
    float    farPlane;

    float4x4 getViewProj()
    {
        return mul(proj, view);
    }

    // Get the linear depth (view Z) for a depth value in [0, 1] for a perspective projection.
    float getLinearDepth(float depth)
    {
        return (nearPlane * farPlane) / (farPlane - depth * (farPlane - nearPlane));
    }

    // View to World Space conversion for points.
    float3 viewToWorld_Point(float3 viewPosition, GPUCameraData camera)
    {
        return mul(camera.viewInverse, float4(viewPosition, 1.0)).xyz;
    }

    // View to World Space conversion for vectors.
    float3 viewToWorld_Vector(float3 viewDirection, GPUCameraData camera)
    {
        return mul(float3x3(camera.viewInverse), viewDirection);
    }

    // World to View Space conversion for points.
    float3 worldToView_Point(float3 worldPosition, GPUCameraData camera)
    {
        return mul(camera.view, float4(worldPosition, 1.0)).xyz;
    }

    // World to View Space conversion for vectors.
    float3 worldToView_Vector(float3 worldDirection, GPUCameraData camera)
    {
        return mul(float3x3(camera.view), worldDirection);
    }
};
