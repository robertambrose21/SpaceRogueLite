# Shader compilation utilities for SDL_GPU
# Compiles HLSL shaders to SPIRV, DXIL, and MSL formats using SDL_shadercross

# Find shadercross CLI tool (provided by sdl_shadercross Conan package)
# First check if it's already in PATH, then search in Conan cache
find_program(SHADERCROSS shadercross
    HINTS
        ENV PATH
        "$ENV{HOME}/.conan2/p/b/sdl_s*/p/bin"
    DOC "SDL_shadercross CLI tool for shader compilation"
)

# If not found, try to locate it in the Conan cache using glob
if(NOT SHADERCROSS)
    file(GLOB SHADERCROSS_CANDIDATES "$ENV{HOME}/.conan2/p/b/sdl_s*/p/bin/shadercross")
    if(SHADERCROSS_CANDIDATES)
        list(GET SHADERCROSS_CANDIDATES 0 SHADERCROSS)
        message(STATUS "Found shadercross in Conan cache: ${SHADERCROSS}")
    endif()
endif()

if(NOT SHADERCROSS)
    message(FATAL_ERROR
        "shadercross not found!\n"
        "Please either:\n"
        "  1. Source the Conan build environment before running CMake:\n"
        "     source <build_dir>/generators/conanbuild.sh\n"
        "  2. Or ensure sdl_shadercross is built:\n"
        "     cd third-party/sdl_shadercross && conan create . -s build_type=Release"
    )
endif()

message(STATUS "Found shadercross: ${SHADERCROSS}")

# Get the library path for DXC shared libraries (same directory structure as bin)
get_filename_component(SHADERCROSS_BIN_DIR "${SHADERCROSS}" DIRECTORY)
get_filename_component(SHADERCROSS_PKG_DIR "${SHADERCROSS_BIN_DIR}" DIRECTORY)
set(SHADERCROSS_LIB_DIR "${SHADERCROSS_PKG_DIR}/lib")

# Build LD_LIBRARY_PATH for running shadercross
set(SHADERCROSS_LD_PATH "${SHADERCROSS_LIB_DIR}")

# Also check for Conan-managed pulseaudio and other SDL dependencies
file(GLOB PULSE_LIB_DIRS "$ENV{HOME}/.conan2/p/b/pulse*/p/lib" "$ENV{HOME}/.conan2/p/b/pulse*/p/lib/pulseaudio")
file(GLOB SNDIO_LIB_DIRS "$ENV{HOME}/.conan2/p/b/libsn*/p/lib")
foreach(LIB_DIR ${PULSE_LIB_DIRS} ${SNDIO_LIB_DIRS})
    set(SHADERCROSS_LD_PATH "${SHADERCROSS_LD_PATH}:${LIB_DIR}")
endforeach()

message(STATUS "Shadercross LD_LIBRARY_PATH: ${SHADERCROSS_LD_PATH}")

# Helper function to convert binary file to C array
function(bin_to_c_array INPUT_FILE OUTPUT_FILE ARRAY_NAME)
    file(READ ${INPUT_FILE} HEX_CONTENT HEX)
    string(LENGTH "${HEX_CONTENT}" HEX_LENGTH)
    math(EXPR BYTE_COUNT "${HEX_LENGTH} / 2")

    set(C_ARRAY "static const unsigned char ${ARRAY_NAME}[${BYTE_COUNT}] = {\n")

    set(COUNTER 0)
    string(REGEX MATCHALL ".." HEX_BYTES "${HEX_CONTENT}")
    foreach(HEX_BYTE ${HEX_BYTES})
        if(COUNTER GREATER 0)
            string(APPEND C_ARRAY ",")
        endif()

        math(EXPR LINE_POS "${COUNTER} % 12")
        if(LINE_POS EQUAL 0)
            string(APPEND C_ARRAY "\n    ")
        endif()

        string(APPEND C_ARRAY "0x${HEX_BYTE}")
        math(EXPR COUNTER "${COUNTER} + 1")
    endforeach()

    string(APPEND C_ARRAY "\n}\\;\n\n")
    file(WRITE ${OUTPUT_FILE} ${C_ARRAY})
endfunction()

# Main function to compile shaders to all formats and generate header
function(compile_shader_to_header)
    set(options "")
    set(oneValueArgs VERTEX_SHADER FRAGMENT_SHADER OUTPUT_HEADER SHADER_NAME)
    set(multiValueArgs "")
    cmake_parse_arguments(SHADER "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT SHADER_VERTEX_SHADER OR NOT SHADER_FRAGMENT_SHADER OR NOT SHADER_OUTPUT_HEADER)
        message(FATAL_ERROR "Missing required arguments to compile_shader_to_header")
    endif()

    get_filename_component(SHADER_DIR "${SHADER_OUTPUT_HEADER}" DIRECTORY)
    file(MAKE_DIRECTORY "${SHADER_DIR}")

    set(TEMP_DIR "${CMAKE_BINARY_DIR}/shader_temp/${SHADER_SHADER_NAME}")
    file(MAKE_DIRECTORY "${TEMP_DIR}")

    # Compile HLSL to SPIRV
    set(SPIRV_VERT "${TEMP_DIR}/shader.vert.spv")
    set(SPIRV_FRAG "${TEMP_DIR}/shader.frag.spv")

    # Set up environment for shadercross (needs LD_LIBRARY_PATH for DXC libraries)
    set(SHADER_ENV "LD_LIBRARY_PATH=${SHADERCROSS_LD_PATH}:$ENV{LD_LIBRARY_PATH}")

    message(STATUS "Compiling ${SHADER_VERTEX_SHADER} to SPIRV...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env "${SHADER_ENV}" ${SHADERCROSS} ${SHADER_VERTEX_SHADER} -s HLSL -d SPIRV -t vertex -e main -o ${SPIRV_VERT}
        RESULT_VARIABLE RESULT
        ERROR_VARIABLE ERROR_OUTPUT
    )
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to compile vertex shader to SPIRV: ${ERROR_OUTPUT}")
    endif()

    message(STATUS "Compiling ${SHADER_FRAGMENT_SHADER} to SPIRV...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env "${SHADER_ENV}" ${SHADERCROSS} ${SHADER_FRAGMENT_SHADER} -s HLSL -d SPIRV -t fragment -e main -o ${SPIRV_FRAG}
        RESULT_VARIABLE RESULT
        ERROR_VARIABLE ERROR_OUTPUT
    )
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to compile fragment shader to SPIRV: ${ERROR_OUTPUT}")
    endif()

    # Convert SPIRV to C arrays
    set(SPIRV_VERT_C "${TEMP_DIR}/spirv_vertex.c")
    set(SPIRV_FRAG_C "${TEMP_DIR}/spirv_fragment.c")
    bin_to_c_array(${SPIRV_VERT} ${SPIRV_VERT_C} "${SHADER_SHADER_NAME}_spirv_vertex")
    bin_to_c_array(${SPIRV_FRAG} ${SPIRV_FRAG_C} "${SHADER_SHADER_NAME}_spirv_fragment")

    # Start building the header file
    set(HEADER_CONTENT "// Generated shader header for ${SHADER_SHADER_NAME}\n")
    string(APPEND HEADER_CONTENT "// DO NOT EDIT - Generated by CMake using SDL_shadercross\n\n")
    string(APPEND HEADER_CONTENT "#pragma once\n\n")

    # Write header preamble
    file(WRITE ${SHADER_OUTPUT_HEADER} ${HEADER_CONTENT})

    # Append SPIRV arrays directly from files
    file(READ ${SPIRV_VERT_C} SPIRV_VERT_CONTENT)
    file(APPEND ${SHADER_OUTPUT_HEADER} "${SPIRV_VERT_CONTENT}\n")
    file(READ ${SPIRV_FRAG_C} SPIRV_FRAG_CONTENT)
    file(APPEND ${SHADER_OUTPUT_HEADER} "${SPIRV_FRAG_CONTENT}\n")

    # Compile HLSL to DXIL (for Direct3D 12)
    message(STATUS "Compiling to DXIL...")
    set(DXIL_VERT "${TEMP_DIR}/shader.vert.dxil")
    set(DXIL_FRAG "${TEMP_DIR}/shader.frag.dxil")

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env "${SHADER_ENV}" ${SHADERCROSS} ${SHADER_VERTEX_SHADER} -s HLSL -d DXIL -t vertex -e main -o ${DXIL_VERT}
        RESULT_VARIABLE RESULT
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env "${SHADER_ENV}" ${SHADERCROSS} ${SHADER_FRAGMENT_SHADER} -s HLSL -d DXIL -t fragment -e main -o ${DXIL_FRAG}
        RESULT_VARIABLE RESULT
    )

    if(EXISTS ${DXIL_VERT} AND EXISTS ${DXIL_FRAG})
        set(DXIL_VERT_C "${TEMP_DIR}/dxil_vertex.c")
        set(DXIL_FRAG_C "${TEMP_DIR}/dxil_fragment.c")
        bin_to_c_array(${DXIL_VERT} ${DXIL_VERT_C} "${SHADER_SHADER_NAME}_dxil_vertex")
        bin_to_c_array(${DXIL_FRAG} ${DXIL_FRAG_C} "${SHADER_SHADER_NAME}_dxil_fragment")

        file(READ ${DXIL_VERT_C} DXIL_VERT_CONTENT)
        file(APPEND ${SHADER_OUTPUT_HEADER} "${DXIL_VERT_CONTENT}\n")
        file(READ ${DXIL_FRAG_C} DXIL_FRAG_CONTENT)
        file(APPEND ${SHADER_OUTPUT_HEADER} "${DXIL_FRAG_CONTENT}\n")
    endif()

    # Compile HLSL to MSL (for Metal on macOS/iOS)
    message(STATUS "Compiling to MSL...")
    set(MSL_VERT "${TEMP_DIR}/shader.vert.msl")
    set(MSL_FRAG "${TEMP_DIR}/shader.frag.msl")

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env "${SHADER_ENV}" ${SHADERCROSS} ${SHADER_VERTEX_SHADER} -s HLSL -d MSL -t vertex -e main -o ${MSL_VERT}
        RESULT_VARIABLE RESULT
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env "${SHADER_ENV}" ${SHADERCROSS} ${SHADER_FRAGMENT_SHADER} -s HLSL -d MSL -t fragment -e main -o ${MSL_FRAG}
        RESULT_VARIABLE RESULT
    )

    # On macOS, compile MSL to metallib
    if(APPLE AND EXISTS ${MSL_VERT} AND EXISTS ${MSL_FRAG})
        find_program(METAL_COMPILER xcrun)
        if(METAL_COMPILER)
            set(METALLIB_VERT "${TEMP_DIR}/shader.vert.air")
            set(METALLIB_FRAG "${TEMP_DIR}/shader.frag.air")

            execute_process(
                COMMAND xcrun -sdk macosx metal -c ${MSL_VERT} -o ${METALLIB_VERT}
                RESULT_VARIABLE RESULT
            )
            execute_process(
                COMMAND xcrun -sdk macosx metal -c ${MSL_FRAG} -o ${METALLIB_FRAG}
                RESULT_VARIABLE RESULT
            )

            if(EXISTS ${METALLIB_VERT} AND EXISTS ${METALLIB_FRAG})
                set(METALLIB_VERT_C "${TEMP_DIR}/metallib_vertex.c")
                set(METALLIB_FRAG_C "${TEMP_DIR}/metallib_fragment.c")
                bin_to_c_array(${METALLIB_VERT} ${METALLIB_VERT_C} "${SHADER_SHADER_NAME}_metallib_vertex")
                bin_to_c_array(${METALLIB_FRAG} ${METALLIB_FRAG_C} "${SHADER_SHADER_NAME}_metallib_fragment")

                file(READ ${METALLIB_VERT_C} METALLIB_VERT_CONTENT)
                file(APPEND ${SHADER_OUTPUT_HEADER} "${METALLIB_VERT_CONTENT}\n")
                file(READ ${METALLIB_FRAG_C} METALLIB_FRAG_CONTENT)
                file(APPEND ${SHADER_OUTPUT_HEADER} "${METALLIB_FRAG_CONTENT}\n")
            endif()
        endif()
    endif()

    message(STATUS "Generated shader header: ${SHADER_OUTPUT_HEADER}")
endfunction()
