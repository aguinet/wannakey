set(SRC_FILES
  bigint.cpp
  filesystem.cpp
  process.cpp
  search_primes.cpp
  tools.cpp
  wcry.cpp
)

add_library(wkey STATIC ${SRC_FILES})
target_link_libraries(wkey Psapi.lib shlwapi.lib)
