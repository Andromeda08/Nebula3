slangc Hair.task.slang       -profile glsl_460+spirv_1_6 -target spirv -stage amplification -entry main -o ./bin/hair.task.spv
slangc Hair.mesh.slang       -profile glsl_460+spirv_1_6 -target spirv -stage mesh          -entry main -o ./bin/hair.mesh.spv
slangc Hair.frag.slang       -profile glsl_460+spirv_1_6 -target spirv -stage fragment      -entry main -o ./bin/hair.frag.spv

slangc HybridHair.task.slang -profile glsl_460+spirv_1_6 -target spirv -stage amplification -entry main -o ./bin/HybridHair.task.spv
slangc HybridHair.mesh.slang -profile glsl_460+spirv_1_6 -target spirv -stage mesh          -entry main -o ./bin/HybridHair.mesh.spv
slangc HybridHair.frag.slang -profile glsl_460+spirv_1_6 -target spirv -stage fragment      -entry main -o ./bin/HybridHair.frag.spv

slangc HybridHair_PrepareIndirect.comp.slang -profile glsl_460+spirv_1_6 -target spirv -stage compute -entry main -o ./bin/HybridHair_PrepareIndirect.comp.spv
slangc HybridHair_Visibility.comp.slang -profile glsl_460+spirv_1_6 -target spirv -stage compute -entry main -o ./bin/HybridHair_Visibility.comp.spv
slangc HybridHair_Resolve.comp.slang -profile glsl_460+spirv_1_6 -target spirv -stage compute -entry main -o ./bin/HybridHair_Resolve.comp.spv

slangc Rasterizer.comp.slang -profile glsl_460+spirv_1_6 -target spirv -stage compute       -entry main -o ./bin/Rasterizer.comp.spv
slangc Resolve.comp.slang -profile glsl_460+spirv_1_6 -target spirv -stage compute          -entry main -o ./bin/Resolve.comp.spv