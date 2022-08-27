cmake_minimum_required(VERSION 3.8)


find_package(CUDA 6.0 REQUIRED)
find_package(OptiX REQUIRED)

set_source_files_properties(optixRaycastingKernels.cu
	PROPERTIES CUDA_SOURCE_PROPERTY_FORMAT OBJ
	)

set( source_files
	Main.cpp
	RaycasterOptix.cu
	optixRaycastingKernels.cu
	)

CUDA_GET_SOURCES_AND_OPTIONS(source_files cmake_options options ${source_files})

CUDA_WRAP_SRCS(RaycasterOptix PTX generated_files ${source_files} ${cmake_options} OPTIONS ${options} )
	
add_sandbox(RaycasterOptix
	Main.cpp
	RaycasterOptix.cu
	optixRaycastingKernels.cu
	${generated_files}
	)

target_include_directories(RaycasterOptix
	PRIVATE ${CUDA_INCLUDE_DIRS}
	PRIVATE ${OptiX_INCLUDE}
	PRIVATE ${OptiX_INCLUDE}/optixu
	)

target_link_libraries(RaycasterOptix
	optix
	${CUDA_LIBRARIES}
	)

