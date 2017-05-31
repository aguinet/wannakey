if (MSVC)
  add_executable(wannakey wannakey.cpp)
  target_link_libraries(wannakey wkey)

  add_executable(winapi_check winapi_check.cpp)
  target_link_libraries(winapi_check wkey version.lib)
endif()

add_executable(bench bench.cpp)
target_link_libraries(bench wkey)
