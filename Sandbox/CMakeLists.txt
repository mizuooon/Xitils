cmake_minimum_required(VERSION 3.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../CMake")
find_library(lib_cinderd NAMES cinderd)
find_library(lib_cinder_imguid NAMES cinder_imguid)
find_library(lib_cinder NAMES cinder)
find_library(lib_cinder_imgui NAMES cinder_imgui)

project(XitilsSandbox)

set(XITILS_INCLUDE_DIR ${CMAKE_SOURCE_DIR}../../Include)

add_library(Xitils INTERFACE)
target_sources(Xitils INTERFACE
	${XITILS_INCLUDE_DIR}/Xitils/AccelerationStructure.h
	${XITILS_INCLUDE_DIR}/Xitils/App.h
	${XITILS_INCLUDE_DIR}/Xitils/Bounds.h
	${XITILS_INCLUDE_DIR}/Xitils/Camera.h
	${XITILS_INCLUDE_DIR}/Xitils/Geometry.h
	${XITILS_INCLUDE_DIR}/Xitils/RenderTarget.h
	${XITILS_INCLUDE_DIR}/Xitils/Interaction.h
	${XITILS_INCLUDE_DIR}/Xitils/Material.h
	${XITILS_INCLUDE_DIR}/Xitils/Matrix.h
	${XITILS_INCLUDE_DIR}/Xitils/Object.h
	${XITILS_INCLUDE_DIR}/Xitils/Primitive.h
	${XITILS_INCLUDE_DIR}/Xitils/Ray.h
	${XITILS_INCLUDE_DIR}/Xitils/Sampler.h
	${XITILS_INCLUDE_DIR}/Xitils/Scene.h
	${XITILS_INCLUDE_DIR}/Xitils/Shape.h
	${XITILS_INCLUDE_DIR}/Xitils/SkySphere.h
	${XITILS_INCLUDE_DIR}/Xitils/Transform.h
	${XITILS_INCLUDE_DIR}/Xitils/TriangleMesh.h
	${XITILS_INCLUDE_DIR}/Xitils/Utils.h
	${XITILS_INCLUDE_DIR}/Xitils/Vector.h
	)
target_include_directories(Xitils
	INTERFACE ${XITILS_INCLUDE_DIR}
	)

function(add_sandbox TargetName)
	add_executable(${TargetName} ${ARGN})
	target_link_libraries(${TargetName}
		Xitils
		debug ${lib_cinderd}
		debug ${lib_cinder_imguid}
		optimized ${lib_cinder}
		optimized ${lib_cinder_imgui}
		)
	target_include_directories(${TargetName}
		PRIVATE ${XITILS_INCLUDE_DIR}
		)
	set_target_properties(
	${TargetName} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/../Data"
	)
	#set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${TargetName})
	
	# なんかC++のバージョン指定以下でやるとうまくいかない
	#target_compile_features(${TargetName} PRIVATE cxx_std_17)
endfunction(add_sandbox)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" )

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
set(CMAKE_CXX_FLAGS_RELEASE "/MT")
set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi -DDEBUG")
set(CMAKE_CXX_FLAGS "/openmp")
add_definitions(-D_UNICODE)
add_definitions(-DNOMINMAX)
add_definitions(/MP)

# add_subdirectory(SimpleWindow)
add_subdirectory(RaycasterBVH)
# add_subdirectory(RaycasterEmbree)
# add_subdirectory(RaycasterOptix)