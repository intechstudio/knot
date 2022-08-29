cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(UnitTest VERSION 1.0)

# add the executable
add_executable(${PROJECT_NAME} main.c unity.c ../midi_translator.c)