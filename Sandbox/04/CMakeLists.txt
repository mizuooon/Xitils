cmake_minimum_required(VERSION 3.8)

set(SandboxIndex 04)

set(ProjectName Sandbox-${SandboxIndex})
project(${ProjectName})

add_executable(${ProjectName} ${SandboxIndex}.cpp)


find_library(lib_embree3 NAMES embree3)
find_library(lib_tbb NAMES tbb)
find_library(lib_tbbmalloc tbbmalloc)

target_link_libraries(${ProjectName}
	${lib_embree3}
	${lib_tbb}
	${lib_tbbmalloc}
	)

include(../CMakeCommon.txt)

