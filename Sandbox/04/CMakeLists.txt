cmake_minimum_required(VERSION 3.8)

find_package(embree 3 REQUIRED)

add_sandbox(04
	04.cpp
	)

target_include_directories(04
	PRIVATE ${EMBREE_INCLUDE_DIRS}
	)
	
target_link_libraries(04
	${EMBREE_LIBRARIES}
	)
