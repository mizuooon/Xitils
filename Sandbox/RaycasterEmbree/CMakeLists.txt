cmake_minimum_required(VERSION 3.8)

find_package(embree 3 REQUIRED)

add_sandbox(RaycasterEmbree
	Main.cpp
	)

target_include_directories(RaycasterEmbree
	PRIVATE ${EMBREE_INCLUDE_DIRS}
	)
	
target_link_libraries(RaycasterEmbree
	${EMBREE_LIBRARIES}
	)
