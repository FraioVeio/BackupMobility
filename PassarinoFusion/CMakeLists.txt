cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(PassarinoIntegration)

add_executable(PassarinoFusion main.cpp)

target_link_libraries (PassarinoFusion  mosquitto)
