cmake_minimum_required (VERSION 3.5.0)

project(CommApp-Client)

set(prefix /usr)

set(tools ${prefix})
set(CMAKE_C_COMPILER ${tools}/bin/gcc)

include_directories(
	${prefix}/include/glib-2.0 
	${prefix}/lib/x86_64-linux-gnu/glib-2.0/include
)

set(
	CMAKE_LIBRARY_PATH 
	${prefix}/lib 
	${prefix}/lib/x86_64-linux-gnu
)

set(
	GLIB
	${CMAKE_LIBRARY_PATH}/libglib-2.0.so
)

set(target ${CMAKE_PROJECT_NAME})
add_executable(${target} main.c comm_module.c)
target_link_libraries(${target} ${GLIB})