find_program(CARGO cargo REQUIRED)

function(nrt_build)
    message("[nrt] Building nrt")
    execute_process(
        COMMAND ${CARGO} build --release --target-dir ${CMAKE_BINARY_DIR}/nrt
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/Tools/nrt
        RESULT_VARIABLE CARGO_RESULT
    )
endfunction()