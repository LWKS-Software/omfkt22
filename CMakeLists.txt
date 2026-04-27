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

# Include directories (matching Xcode HEADER_SEARCH_PATHS)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/portinc
    ${CMAKE_CURRENT_SOURCE_DIR}/kitomfi
    ${CMAKE_CURRENT_SOURCE_DIR}/bento
)

# Windows/Linux need additional paths for jpeg/avidjpg
if(NOT APPLE)
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/jpeg
        ${CMAKE_CURRENT_SOURCE_DIR}/avidjpg
    )
endif()

# Windows-specific includes
if(WIN32)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/compat)
endif()

# Platform-specific preprocessor definitions
if(WIN32)
    add_definitions(
        -DHAVE_STDC
        -DOMFI_JPEG_CODEC
        -DOMFI_ENABLE_SEMCHECK
        -DOMFI_ERROR_TRACE
        -DOMFI_ENABLE_STREAM_CACHE
        -DINCLUDES_ARE_ANSI
        -DOMF_BUILDING_TOOLKIT_DLL
        -DOMFI_NEED_ULONG
        -DOMF_USE_DLL
        -DWIN32
    )
elseif(APPLE)
    # Match Xcode GCC_PREPROCESSOR_DEFINITIONS exactly
    add_definitions(
        -DPORTKEY_INT64_TYPE=long
        -DPORTKEY_INT64_NATIVE=1
        -DPORTKEY_OS_MACSYS=1
        -DLW_MAC=1
    )
    # Debug-only definitions (matching Xcode Debug config)
    add_compile_definitions($<$<CONFIG:Debug>:DEBUG=1>)
    add_compile_definitions($<$<CONFIG:Debug>:OMFI_ERROR_TRACE>)
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

# Set C standard for macOS (matching Xcode GCC_C_LANGUAGE_STANDARD = gnu99)
if(APPLE)
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED YES)
endif()

# Explicit source lists matching Xcode project sources exactly
set(BENTO_SOURCES
    bento/BufferIO.c
    bento/CMCntOps.c
    bento/CMDbgOps.c
    bento/CMErrOps.c
    bento/CMHndOps.c
    bento/CMObjOps.c
    bento/CMRefOps.c
    bento/CMSesOps.c
    bento/CMTPOps.c
    bento/CMVHOps.c
    bento/CMValOps.c
    bento/DynValus.c
    bento/FreeSpce.c
    bento/GlbNames.c
    bento/Handlers.c
    bento/ListMgr.c
    bento/SymTbMgr.c
    bento/TOCEnts.c
    bento/TOCIO.c
    bento/TOCObjs.c
    bento/Update.c
    bento/Utility.c
    bento/Values.c
    bento/oldtoc.c
)

set(KITOMFI_SOURCES
    kitomfi/omAcces.c
    kitomfi/omcAIFF.c
    kitomfi/omcCDCI.c
    kitomfi/omCheck.c
    kitomfi/omcJPEG.c
    kitomfi/omCodec.c
    kitomfi/omCompos.c
    kitomfi/omcRGBA.c
    kitomfi/omcStd.c
    kitomfi/omcStrm.c
    kitomfi/omcTIFF.c
    kitomfi/omCvt.c
    kitomfi/omcWAVE.c
    kitomfi/omcXlate.c
    kitomfi/omEffect.c
    kitomfi/omErr.c
    kitomfi/omfansic.c
    kitomfi/omFile.c
    kitomfi/omFndSrc.c
    kitomfi/omfsess.c
    kitomfi/omIEEE.c
    kitomfi/omJFIF.c
    kitomfi/omJPEG.c
    kitomfi/omLocate.c
    kitomfi/omMedia.c
    kitomfi/omMedObj.c
    kitomfi/omMobBld.c
    kitomfi/ommobget.c
    kitomfi/omMobMgt.c
    kitomfi/omSearch.c
    kitomfi/omTable.c
    kitomfi/omUtils.c
)

set(AVIDJPG_SOURCES
    avidjpg/omcAvidJFIF.c
    avidjpg/omcAvJPED.c
    avidjpg/omcAVR.c
    avidjpg/omJPED.c
)

set(JPEG_SOURCES
    jpeg/jbsmooth.c
    jpeg/jcarith.c
    jpeg/jccolor.c
    jpeg/jcdeflts.c
    jpeg/jcexpand.c
    jpeg/jchuff.c
    jpeg/jcmaster.c
    jpeg/jcmcu.c
    jpeg/jcpipe.c
    jpeg/jcsample.c
    jpeg/jdarith.c
    jpeg/jdcolor.c
    jpeg/jddeflts.c
    jpeg/jdhuff.c
    jpeg/jdmaster.c
    jpeg/jdmcu.c
    jpeg/jdpipe.c
    jpeg/jdsample.c
    jpeg/jerror.c
    jpeg/jfwddct.c
    jpeg/jmemmgr.c
    jpeg/jmemsys.c
    jpeg/jquant1.c
    jpeg/jquant2.c
    jpeg/jrevdct.c
    jpeg/jutils.c
)

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
        MACOSX_DEPLOYMENT_TARGET "11.0"
    )
    # Link CoreServices and Carbon frameworks
    target_link_libraries(omftoolkit
        "-framework CoreServices"
        "-framework Carbon"
    )
    # Add framework search paths for subframeworks (HIToolbox, CarbonCore)
    # This matches Xcode's FRAMEWORK_SEARCH_PATHS setting
    target_compile_options(omftoolkit PRIVATE
        "-F${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks"
        "-F${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks"
    )
else()
    set_target_properties(omftoolkit PROPERTIES
        OUTPUT_NAME "omftoolkit"
    )
endif()

# Installation
install(TARGETS omftoolkit
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)
