cmake_minimum_required(VERSION 3.16)
project(falcon_artifact LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 기본 빌드 타입
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# 실행파일을 repo 루트/bin 으로 떨어뜨리기
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${REPO_ROOT}/bin")

# 경고 옵션 (GNU/Clang일 때만)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wshadow)
endif()

# --- PQClean 경로 설정 ---
set(PQCLEAN_DIR "${CMAKE_CURRENT_LIST_DIR}/pqclean")

set(PQCLEAN_SOURCES
    ${PQCLEAN_DIR}/crypto_sign/falcon-512/clean/fft.c
    ${PQCLEAN_DIR}/crypto_sign/falcon-512/clean/fpr.c
    ${PQCLEAN_DIR}/crypto_sign/falcon-512/clean/keygen.c
    ${PQCLEAN_DIR}/crypto_sign/falcon-512/clean/codec.c
    ${PQCLEAN_DIR}/crypto_sign/falcon-512/clean/vrfy.c
    ${PQCLEAN_DIR}/crypto_sign/falcon-512/clean/common.c
    ${PQCLEAN_DIR}/common/fips202.c
)

# 공통 라이브러리(코어) 만들기: core + wrapper + pqclean
# NOTE: core/wrapper 소스가 없으므로 현재는 비활성화
# add_library(falconcore STATIC
#   core/attack.c
#   wrapper/falcon_wrap.c
#   ${PQCLEAN_SOURCES}
# )
#
# add_library(postproc STATIC
#   core/attack.c
#   core/postproc.c
#   wrapper/falcon_wrap.c
#   ${PQCLEAN_SOURCES}
# )
#
# target_include_directories(falconcore PUBLIC
#   ${CMAKE_CURRENT_LIST_DIR}/include
#   ${CMAKE_CURRENT_LIST_DIR}/core
#   ${CMAKE_CURRENT_LIST_DIR}/wrapper
#   ${PQCLEAN_DIR}
# )
#
# # 필요하면 수학 라이브러리 링크(리눅스에서 종종 필요)
# target_link_libraries(falconcore PUBLIC m)

# ------------------------
# 실행파일들 (원하는 만큼 add_executable 추가)
# ------------------------
add_executable(make_input
  Inputdata/make_input.c
  leak.c
  ${PQCLEAN_SOURCES}
)
target_include_directories(make_input PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${PQCLEAN_DIR}
  ${PQCLEAN_DIR}/common
)

# make_group (post-capture grouping utility)
add_executable(make_group
  Inputdata/make_group.c
)

# post-processing (Falcon SPA)
add_executable(falcon_postproc
  PostProcessing/main.cxx
  PostProcessing/fft.cxx
)
target_include_directories(falcon_postproc PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/PostProcessing
)

add_executable(falcon_postproc_test
  PostProcessing/main_test.cxx
  PostProcessing/fft.cxx
)
target_include_directories(falcon_postproc_test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/PostProcessing
)
