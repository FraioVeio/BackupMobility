cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(Steering)

add_executable(Steering main.cpp)

target_link_libraries (Steering  mosquitto)
