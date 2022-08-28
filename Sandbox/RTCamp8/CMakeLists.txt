cmake_minimum_required(VERSION 3.8)

add_sandbox(RTCamp8
	Main.cpp
	)

target_link_libraries(RTCamp8 PRIVATE
	OpenImageDenoise
	)
