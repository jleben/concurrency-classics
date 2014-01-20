cmake_minimum_required( VERSION 2.8.10 )

project(csc564_a1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

macro(add_app app)
  add_executable(${app} ${ARGN})
  target_link_libraries(${app} pthread)
  set_property(TARGET ${app} APPEND PROPERTY LINK_FLAGS "-Wl,--no-as-needed")
endmacro()


add_app(philosophers cpp/dining_philosophers.cpp)

add_app(smokers cpp/cigarette_smokers.cpp)

add_app(barber cpp/sleeping_barber.cpp)
