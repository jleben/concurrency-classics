cmake_minimum_required( VERSION 2.8.10 )

project(csc564_a1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

macro(add_app app)
  add_executable(${app} ${ARGN})
  target_link_libraries(${app} pthread)
  set_property(TARGET ${app} APPEND PROPERTY LINK_FLAGS "-Wl,--no-as-needed")
endmacro()


add_app(philosophers cpp/dining_philosophers.cpp)
add_app(philosophers-csp cpp/dining_philosophers_csp.cpp)

add_app(smokers cpp/cigarette_smokers.cpp)
add_app(smokers-csp cpp/cigarette_smokers_csp.cpp)

add_app(barber cpp/sleeping_barber.cpp)
add_app(barber-csp cpp/sleeping_barber_csp.cpp)

add_app(santa cpp/santa_claus.cpp)
add_app(santa-csp cpp/santa_claus_csp.cpp)

add_app(h2o cpp/h2o.cpp)
add_app(h2o-csp cpp/h2o_csp.cpp)

add_app(river cpp/river_crossing.cpp)
add_app(river-csp cpp/river_crossing_csp.cpp)

add_app(test-condition cpp/test_condition_variable.cpp)
