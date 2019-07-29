cmake_minimum_required(VERSION 3.8)


find_package(CUDA 6.0 REQUIRED)
find_package(OptiX REQUIRED)

set_source_files_properties(optixRaycastingKernels.cu
	PROPERTIES CUDA_SOURCE_PROPERTY_FORMAT OBJ
	)

set( source_files
	05.cpp
	05.cu
	optixRaycastingKernels.cu
	)

CUDA_GET_SOURCES_AND_OPTIONS(source_files cmake_options options ${source_files})

CUDA_WRAP_SRCS(05 PTX generated_files ${source_files} ${cmake_options} OPTIONS ${options} )
	
add_sandbox(05
	05.cpp
	05.cu
	optixRaycastingKernels.cu
	${generated_files}
	)

target_include_directories(05
	PRIVATE ${CUDA_INCLUDE_DIRS}
	PRIVATE ${OptiX_INCLUDE}
	PRIVATE ${OptiX_INCLUDE}/optixu
	)

target_link_libraries(05
	optix
	${CUDA_LIBRARIES}
	)

