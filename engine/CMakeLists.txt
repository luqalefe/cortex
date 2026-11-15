cmake_minimum_required(VERSION 3.20)
project(codex_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Faust: localizar os headers de arquitetura (dsp.h, UI.h, MapUI.h...)
find_program(FAUST_BIN faust)
execute_process(COMMAND brew --prefix faust OUTPUT_VARIABLE FAUST_PREFIX
                OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(FAUST_PREFIX)
  set(FAUST_INCLUDE "${FAUST_PREFIX}/include")
endif()

# Gerar o C++ do DSP a partir do .dsp (fonte única do som)
set(DSP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../dsp/codex_delay.dsp)
set(DSP_OUT ${CMAKE_CURRENT_SOURCE_DIR}/generated/codex_delay.hpp)
if(FAUST_BIN)
  add_custom_command(
    OUTPUT ${DSP_OUT}
    COMMAND ${FAUST_BIN} -lang cpp -cn CodexDelay -o ${DSP_OUT} ${DSP_SRC}
    DEPENDS ${DSP_SRC}
    COMMENT "Faust: gerando codex_delay.hpp")
  add_custom_target(gen_dsp DEPENDS ${DSP_OUT})
endif()

add_executable(codex-engine
  src/main.cpp
  third_party/miniaudio_impl.cpp)
if(FAUST_BIN)
  add_dependencies(codex-engine gen_dsp)
endif()

target_include_directories(codex-engine PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party
  ${FAUST_INCLUDE})

find_library(COREAUDIO CoreAudio)
find_library(AUDIOUNIT AudioUnit)
find_library(AUDIOTOOLBOX AudioToolbox)
find_library(COREFOUNDATION CoreFoundation)
target_link_libraries(codex-engine PRIVATE
  ${COREAUDIO} ${AUDIOUNIT} ${AUDIOTOOLBOX} ${COREFOUNDATION})
