cmake_minimum_required(VERSION 3.8)

set(SandboxIndex 02)

set(ProjectName Sandbox-${SandboxIndex})
project(${ProjectName})

add_executable(${ProjectName} ${SandboxIndex}.cpp)

include(../CMakeCommon.txt)
