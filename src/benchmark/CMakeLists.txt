
add_executable(fbksd-benchmark main.cpp)
target_link_libraries(fbksd-benchmark PRIVATE fbksd::libbenchmark)
install(TARGETS fbksd-benchmark
    RUNTIME DESTINATION bin
)
