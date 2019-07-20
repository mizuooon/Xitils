cmake_minimum_required(VERSION 3.8)

find_library(lib_cinderd NAMES cinderd)
find_library(lib_cinder_imguid NAMES cinder_imguid)
find_library(lib_cinder NAMES cinder)
find_library(lib_cinder_imgui NAMES cinder_imgui)

project(XitilsSandbox)

function(add_sandbox TargetName)
	add_executable(${TargetName} ${TargetName}.cpp)
	target_link_libraries(${TargetName}
		debug ${lib_cinderd}
		debug ${lib_cinder_imguid}
		optimized ${lib_cinder}
		optimized ${lib_cinder_imgui}
		)
	target_include_directories(${TargetName}
		PRIVATE ../../Include
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
set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
set(CMAKE_CXX_FLAGS "/openmp")
add_definitions(-D_UNICODE)


add_subdirectory(01)
add_subdirectory(02)
add_subdirectory(03)
add_subdirectory(04)