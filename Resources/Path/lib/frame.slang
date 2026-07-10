#pragma once

struct Frame
{
    float3x3 tbn;

    static float cosTheta(float3 w)
    {
        return w.z;
    }

    static Frame build(float3 n)
    {
        float3 t;
        if (abs(n.x) > abs(n.y))
        {
            float invLen = 1.0 / sqrt(n.x * n.x + n.z * n.z);
            t = float3(n.z * invLen, 0.0, -n.x * invLen);
        }
        else
        {
            float invLen = 1.0 / sqrt(n.y * n.y + n.z * n.z);
            t = float3(0.0, n.z * invLen, -n.y * invLen);
        }
        float3 b = cross(t, n);

        Frame frame;
        frame.tbn = float3x3(t, b, n);
        return frame;
    }

    static Frame build(float3 n, float4 tangent)
    {
        float3 t = tangent.xyz;

        if (dot(t, t) < 1e-8)
        {
            return build(n);
        }

        t = normalize(t - n * dot(n, t));
        float3 b = cross(n, t) * tangent.w;

        Frame frame;
        frame.tbn = float3x3(t, b, n);
        return frame;
    }

    float3 toLocal(float3 v)
    {
        return mul(tbn, v);
    }

    float3 toWorld(float3 v)
    {
        return mul(transpose(tbn), v);
    }
};
