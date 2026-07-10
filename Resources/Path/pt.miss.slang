#language slang 2026

#include "lib/common.slang"

[shader("miss")]
void main(inout PathPayload payload)
{
    payload.radiance    += payload.throughput * float3(0.0);
    payload.isTerminated = true;
}