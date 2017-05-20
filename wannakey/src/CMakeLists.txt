set(SRC_FILES
  bigint.cpp
  filesystem.cpp
  main.cpp
  process.cpp
  search_primes.cpp
  tools.cpp
  wcry.cpp
)

add_executable(wannakey ${SRC_FILES})
target_link_libraries(wannakey Psapi.lib shlwapi.lib)
