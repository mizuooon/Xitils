cmake_minimum_required(VERSION 3.8)

set(SANDBOX_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data")

function(add_sandbox TargetName)
	add_executable(${TargetName} ${ARGN})
	target_link_libraries(${TargetName} PRIVATE
		Xitils
		)
	target_include_directories(${TargetName} PRIVATE
		${XITILS_INCLUDE_DIR}
		)
	set_target_properties(
	${TargetName} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${SANDBOX_DATA_DIR}
	)
	#set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${TargetName})
	
	# なんかC++のバージョン指定を以下のようにやるとうまくいかない
	#target_compile_features(${TargetName} PRIVATE cxx_std_17)
endfunction(add_sandbox)


add_subdirectory(SimplePathTracer)
add_subdirectory(SimplePathTracerWithDenoiser)
add_subdirectory(SimpleWindow)
add_subdirectory(MicrofacetBasedNormalMapping)
add_subdirectory(PrefilteringForRenderingDisplacementMappedSurfaces)
add_subdirectory(VisualizeError)
add_subdirectory(VonMisesFisherDistribution)
add_subdirectory(SphericalHarmonics)
add_subdirectory(RTCamp8)

add_subdirectory(_Experimental/RaycasterEmbree)
#add_subdirectory(_Experimental/RaycasterOptix)