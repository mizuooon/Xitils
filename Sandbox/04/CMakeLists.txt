cmake_minimum_required(VERSION 3.8)

add_sandbox(04)

find_library(lib_embree3 NAMES embree3)
find_library(lib_tbb NAMES tbb)
find_library(lib_tbbmalloc tbbmalloc)

target_link_libraries(04
	${lib_embree3}
	${lib_tbb}
	${lib_tbbmalloc}
	)
