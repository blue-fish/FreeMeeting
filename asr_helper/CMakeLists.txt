cmake_minimum_required(VERSION 3.14)
project(SherpaAsrHelper VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sherpa-ONNX C API header (WeChat bundled copy)
set(SHERPA_API_DIR "E:/colinCode/stage_project/Wechat/sherpaApi/include")

# ===== sherpa_asr_helper executable =====
add_executable(sherpa_asr_helper main_win32.cpp)
target_include_directories(sherpa_asr_helper PRIVATE
    ${SHERPA_API_DIR}
)
target_link_libraries(sherpa_asr_helper PRIVATE ws2_32)
target_compile_definitions(sherpa_asr_helper PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE
)

# Copy Sherpa DLL + deps to output dir after build
# NOTE: sherpa-onnx-c-api.dll from release/asr, but onnxruntime.dll MUST come
# from sherpaApi/bin (the newer version that matches the Sherpa DLL build)
set(SHERPA_DLL_SRC "E:/colinCode/stage_project/Wechat/release/asr")
set(ONNX_DLL_SRC "E:/colinCode/stage_project/Wechat/sherpaApi/bin")
add_custom_command(TARGET sherpa_asr_helper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SHERPA_DLL_SRC}/sherpa-onnx-c-api.dll"
        "${ONNX_DLL_SRC}/onnxruntime.dll"
        "${ONNX_DLL_SRC}/onnxruntime_providers_shared.dll"
        "$<TARGET_FILE_DIR:sherpa_asr_helper>"
    COMMENT "Copying Sherpa DLLs to output directory"
)