cmake_minimum_required(VERSION 3.8)

project(Xitils CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake")

set(XITILS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Include)
set(XITILS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Src)

set(CMAKE_CXX_FLAGS_RELEASE "/MT")
set(CMAKE_CXX_FLAGS_DEBUG "/MTd")

add_library(Xitils STATIC
	${XITILS_SOURCE_DIR}/dummy.cpp
	)
target_sources(Xitils PRIVATE
	${XITILS_INCLUDE_DIR}/Xitils/AccelerationStructure.h
	${XITILS_INCLUDE_DIR}/Xitils/App.h
	${XITILS_INCLUDE_DIR}/Xitils/Bounds.h
	${XITILS_INCLUDE_DIR}/Xitils/Camera.h
	${XITILS_INCLUDE_DIR}/Xitils/Geometry.h
	${XITILS_INCLUDE_DIR}/Xitils/Intersection.h
	${XITILS_INCLUDE_DIR}/Xitils/Material.h
	${XITILS_INCLUDE_DIR}/Xitils/Matrix.h
	${XITILS_INCLUDE_DIR}/Xitils/Object.h
	${XITILS_INCLUDE_DIR}/Xitils/PathTracer.h
	${XITILS_INCLUDE_DIR}/Xitils/Ray.h
	${XITILS_INCLUDE_DIR}/Xitils/RenderTarget.h
	${XITILS_INCLUDE_DIR}/Xitils/Sampler.h
	${XITILS_INCLUDE_DIR}/Xitils/Scene.h
	${XITILS_INCLUDE_DIR}/Xitils/Shape.h
	${XITILS_INCLUDE_DIR}/Xitils/SkySphere.h
	${XITILS_INCLUDE_DIR}/Xitils/SphericalHarmonics.h
	${XITILS_INCLUDE_DIR}/Xitils/Texture.h
	${XITILS_INCLUDE_DIR}/Xitils/Transform.h
	${XITILS_INCLUDE_DIR}/Xitils/TriangleIndexed.h
	${XITILS_INCLUDE_DIR}/Xitils/TriangleMesh.h
	${XITILS_INCLUDE_DIR}/Xitils/Utils.h
	${XITILS_INCLUDE_DIR}/Xitils/Vector.h
	${XITILS_INCLUDE_DIR}/Xitils/VonMisesFisherDistribution.h
	)
target_include_directories(Xitils PUBLIC
	${CMAKE_SOURCE_DIR}/ThirdParty
	${XITILS_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/ThirdParty/Cinder/include
	${CMAKE_SOURCE_DIR}/ThirdParty/Cinder-ImGui/include
	${CMAKE_SOURCE_DIR}/ThirdParty/Cinder-ImGui/lib/imgui
	${CMAKE_SOURCE_DIR}/ThirdParty/libsimdpp
	${CMAKE_SOURCE_DIR}/ThirdParty/tinycolormap/include
	)

add_subdirectory(ThirdParty)

target_link_libraries(Xitils PRIVATE
	cinder
	Cinder-ImGui
	)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited configs" FORCE)

set(XITILS_BUILD_SANDBOX 1 CACHE BOOL "")

if(${XITILS_BUILD_SANDBOX})
	enable_language(CXX)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi -DDEBUG")
	set(CMAKE_CXX_STANDARD 20)
	set(CMAKE_CXX_STANDARD_REQUIRED ON) 
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
	set(CMAKE_CXX_FLAGS "/openmp")
	add_definitions(-D_UNICODE)
	add_definitions(-DNOMINMAX)
	add_definitions(/MP)
	add_definitions(/EHsc)

	add_subdirectory(SandBox)
endif()