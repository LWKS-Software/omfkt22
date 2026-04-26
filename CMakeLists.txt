cmake_minimum_required(VERSION 3.15)
project(omftoolkit C)

# Platform detection
if(WIN32)
    set(OMF_PLATFORM_WINDOWS 1)
elseif(APPLE)
    set(OMF_PLATFORM_MAC 1)
else()
    set(OMF_PLATFORM_LINUX 1)
endif()

# Common include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/portinc
    ${CMAKE_CURRENT_SOURCE_DIR}/kitomfi
    ${CMAKE_CURRENT_SOURCE_DIR}/bento
    ${CMAKE_CURRENT_SOURCE_DIR}/jpeg
    ${CMAKE_CURRENT_SOURCE_DIR}/avidjpg
)

# Windows-specific includes
if(WIN32)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/compat)
endif()

# Common definitions
add_definitions(
    -DHAVE_STDC
    -DOMFI_JPEG_CODEC
    -DOMFI_ENABLE_SEMCHECK
    -DOMFI_ERROR_TRACE
    -DOMFI_ENABLE_STREAM_CACHE
    -DINCLUDES_ARE_ANSI
)

# Platform-specific definitions
if(WIN32)
    add_definitions(
        -DOMF_BUILDING_TOOLKIT_DLL
        -DOMFI_NEED_ULONG
        -DOMF_USE_DLL
        -DWIN32
    )
elseif(APPLE)
    add_definitions(
        -DPORTKEY_INT64_TYPE=long
        -DPORTKEY_INT64_NATIVE=1
        -DPORTKEY_OS_MACSYS=1
        -DLW_MAC=1
    )
else()
    add_definitions(
        -DPORTKEY_INT64_NATIVE=1
        -DPORTKEY_INT64_TYPE=long
    )
endif()

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fexceptions -fPIC)
endif()

# Set C standard for macOS
if(APPLE)
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED YES)
endif()

# Collect source files
file(GLOB BENTO_SOURCES "bento/*.c")
file(GLOB KITOMFI_SOURCES "kitomfi/*.c")
file(GLOB JPEG_SOURCES "jpeg/*.c")
file(GLOB AVIDJPG_SOURCES "avidjpg/*.c")

# Exclude certain JPEG files not used in original projects
list(FILTER JPEG_SOURCES EXCLUDE REGEX "jcmain.c|jdmain.c|JPEGexample.c|ansi2knr.c|ckconfig.c")

# Exclude platform-specific JPEG memory files
if(NOT WIN32)
    list(FILTER JPEG_SOURCES EXCLUDE REGEX "jmemdos.c")
endif()
if(NOT(APPLE OR UNIX))
    list(FILTER JPEG_SOURCES EXCLUDE REGEX "jmemansi.c")
endif()

# Create library
add_library(omftoolkit SHARED
    ${BENTO_SOURCES}
    ${KITOMFI_SOURCES}
    ${JPEG_SOURCES}
    ${AVIDJPG_SOURCES}
)

# Platform-specific library settings
if(WIN32)
    # Debug library suffix
    set_target_properties(omftoolkit PROPERTIES
        OUTPUT_NAME "omfToolkit"
        DEBUG_POSTFIX "d"
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )
    set_target_properties(omftoolkit PROPERTIES
        COMPILE_PDB_NAME "omfToolkit"
        COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    # Set runtime library to static
    set_target_properties(omftoolkit PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
elseif(APPLE)
    set_target_properties(omftoolkit PROPERTIES
        OUTPUT_NAME "omftoolkit"
        MACOSX_RPATH 1
        INSTALL_NAME_DIR "@rpath"
    )
    # Set deployment target
    set_target_properties(omftoolkit PROPERTIES
        MACOSX_DEPLOYMENT_TARGET "10.9"
    )
    # Link CoreServices framework
    target_link_libraries(omftoolkit "-framework CoreServices")
else()
    set_target_properties(omftoolkit PROPERTIES
        OUTPUT_NAME "omftoolkit"
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
endif()

# Installation
install(TARGETS omftoolkit
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)
