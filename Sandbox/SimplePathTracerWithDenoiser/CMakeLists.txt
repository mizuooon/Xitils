cmake_minimum_required(VERSION 3.8)

add_sandbox(SimplePathTracerWithDenoiser
	Main.cpp
	)

target_link_libraries(SimplePathTracerWithDenoiser PRIVATE
	OpenImageDenoise
	)
