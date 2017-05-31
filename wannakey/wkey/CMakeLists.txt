set(SRC_FILES
  bigint.cpp
  search_primes.cpp
  tools.cpp
)

if (MSVC)
  set(SRC_FILES
    ${SRC_FILES}
    filesystem.cpp
    wcry.cpp
    process.cpp
  )
endif()

add_library(wkey STATIC ${SRC_FILES})

if (MSVC)
  target_link_libraries(wkey Psapi.lib shlwapi.lib)
endif()
