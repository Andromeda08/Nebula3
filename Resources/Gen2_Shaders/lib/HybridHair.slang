#pragma once

#include "../nbl/Hair.inc.slang"

struct BBoxInt2
{
    int2 bboxMin;
    int2 bboxMax;

    bool isEmpty()
    {
        return any(bboxMin > bboxMax);
    }

    static BBoxInt2 fromTriangle(float2 p0, float2 p1, float2 p2)
    {
        float2 _min = min(min(p0, p1), p2);
        float2 _max = max(max(p0, p1), p2);

        BBoxInt2 result;
        result.bboxMin = int2(floor(_min - 0.5)) + 1;
        result.bboxMax = int2(floor(_max - 0.5));
        return result;
    }

    static BBoxInt2 fromScissor(uint xmin, uint ymin, uint xmax, uint ymax)
    {
        BBoxInt2 result;
        result.bboxMin = int2(xmin, ymin);
        result.bboxMax = int2(xmax, ymax);
        return result;
    }

    BBoxInt2 getClippedBy(BBoxInt2 other)
    {
        BBoxInt2 result;
        result.bboxMin = max(bboxMin, other.bboxMin);
        result.bboxMax = min(bboxMax, other.bboxMax);
        return result;
    }
};

struct Triangle
{
    float2   p0, p1, p2;

    BBoxInt2 bounds;
    float    signedArea;
    float    invArea;
    float    s;

    static Triangle fromInput(ScreenSpaceTriangle t)
    {
        Triangle tri;

        tri.p0 = t.v0.xy;
        tri.p1 = t.v1.xy;
        tri.p2 = t.v2.xy;

        tri.bounds     = BBoxInt2::fromTriangle(tri.p0, tri.p1, tri.p2);
        tri.signedArea = getSignedArea(tri.p0, tri.p1, tri.p2);
        tri.invArea    = 1.0f / tri.signedArea;
        tri.s          = sign(tri.signedArea);

        return tri;
    }

    bool isDegenerate()
    {
        return abs(signedArea) < 0.0000005;
    }

    float3 bary(float2 p)
    {
        float w0 = getSignedArea(p1, p2, p);
        float w1 = getSignedArea(p2, p0, p);
        float w2 = getSignedArea(p0, p1, p);
        return float3(w0, w1, w2) * invArea;
    }
};
