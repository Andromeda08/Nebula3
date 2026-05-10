#language slang 2026

struct TestTriangle
{
    float3 v0;
    float3 v1;
    float3 v2;
    uint   primitiveId;
};

struct PushConstants
{
    TestTriangle*   triangles;
    uint            width;
    uint            height;
    uint            sMinX;
    uint            sMinY;
    uint            sMaxX;
    uint            sMaxY;
    uint            triangleCount;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> pc;

[[vk::binding(0, 0)]]
RWTexture2D<uint64_t> gVisBuffer;

struct BoundingBoxInt2D
{
    int2 bboxMin;
    int2 bboxMax;

    bool isEmpty()
    {
        return any(bboxMin > bboxMax);
    }

    static BoundingBoxInt2D fromTriangle(float2 p0, float2 p1, float2 p2)
    {
        float2 _min = min(min(p0, p1), p2);
        float2 _max = max(max(p0, p1), p2);

        BoundingBoxInt2D result;
        result.bboxMin = int2(floor(_min - 0.5)) + 1;
        result.bboxMax = int2(floor(_max - 0.5));
        return result;
    }

    static BoundingBoxInt2D fromScissor(uint xmin, uint ymin, uint xmax, uint ymax)
    {
        BoundingBoxInt2D result;
        result.bboxMin = int2(xmin, ymin);
        result.bboxMax = int2(xmax, ymax);
        return result;
    }

    BoundingBoxInt2D getClippedBy(BoundingBoxInt2D other)
    {
        BoundingBoxInt2D result;
        result.bboxMin = max(bboxMin, other.bboxMin);
        result.bboxMax = min(bboxMax, other.bboxMax);
        return result;
    }
};

/**
 * Compute the signed area of a triangle using the cross product method
 * Edges AB and AC are computed, then det(AB, AC) * 1/2 gives the signed area.
 */
float getSignedArea(float2 a, float2 b, float2 c)
{
    float2 AB = float2(b.x - a.x, b.y - a.y);
    float2 AC = float2(c.x - a.x, c.y - a.y);
    return 0.5f * determinant(float2x2(AB, AC));
}

struct Triangle
{
    float2 p0, p1, p2;

    BoundingBoxInt2D bounds;
    float            signedArea;
    float            invArea;
    float            s;

    static Triangle fromInput(TestTriangle t)
    {
        Triangle tri;

        tri.p0 = t.v0.xy;
        tri.p1 = t.v1.xy;
        tri.p2 = t.v2.xy;

        tri.bounds     = BoundingBoxInt2D::fromTriangle(tri.p0, tri.p1, tri.p2);
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

[shader("compute")]
[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint triangleIndex = dispatchThreadID.x;
    if (triangleIndex >= pc.triangleCount)
    {
        return;
    }

    BoundingBoxInt2D scissor = BoundingBoxInt2D::fromScissor(pc.sMinX, pc.sMinY, pc.sMaxX, pc.sMaxY);

    TestTriangle t = pc.triangles[triangleIndex];

    // Create Triangle from input
    Triangle tri   = Triangle::fromInput(t);
    if (tri.isDegenerate())
    {
        return;
    }

    // Scissor clipping
    BoundingBoxInt2D clipped = tri.bounds.getClippedBy(scissor);
    if (clipped.isEmpty())
    {
        return;
    }
    
    // Loop over pixels
    for (int y = clipped.bboxMin.y; y <= clipped.bboxMax.y; y++)
    {
        for (int x = clipped.bboxMin.x; x <= clipped.bboxMax.x; x++)
        {
            float2 pixel = float2(x, y) + 0.5;
            float3 bary  = tri.bary(pixel);
            if (any(bary < 0.0f))
            {
                continue;
            }

            float depth = (bary.x * t.v0.z) + (bary.y * t.v1.z) + (bary.z * t.v2.z);

            uint depthBits = asuint(saturate(depth));
            uint64_t packed = (uint64_t(depthBits) << 32) | uint64_t(t.primitiveId);

            InterlockedMin(gVisBuffer[int2(x, y)], packed);
        }
    }
}
