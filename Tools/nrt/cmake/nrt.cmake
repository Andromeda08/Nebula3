find_program(CARGO cargo REQUIRED)

function(nrt_build)
    message("[nrt] Building nrt")
    execute_process(
        COMMAND ${CARGO} build --release --target-dir ${CMAKE_BINARY_DIR}/nst
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/Tools/nst
        RESULT_VARIABLE CARGO_RESULT
    )
endfunction()