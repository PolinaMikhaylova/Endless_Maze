# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\Endless_Maze_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Endless_Maze_autogen.dir\\ParseCache.txt"
  "Endless_Maze_autogen"
  )
endif()
