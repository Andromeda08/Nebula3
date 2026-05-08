slangc Hair.task.slang -profile glsl_460+spirv_1_6 -target spirv -stage amplification -entry main -o ./bin/hair.task.spv
slangc Hair.mesh.slang -profile glsl_460+spirv_1_6 -target spirv -stage mesh          -entry main -o ./bin/hair.mesh.spv
slangc Hair.frag.slang -profile glsl_460+spirv_1_6 -target spirv -stage fragment      -entry main -o ./bin/hair.frag.spv