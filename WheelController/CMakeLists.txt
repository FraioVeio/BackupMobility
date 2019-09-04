cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(WheelController)

add_executable(WheelController main.cpp)

target_link_libraries (WheelController  mosquitto)
