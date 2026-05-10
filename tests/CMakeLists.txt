add_executable(deadzone_test deadzone_test.cpp)
target_compile_features(deadzone_test PRIVATE cxx_std_17)

include(CTest)
add_test(NAME deadzone_test COMMAND deadzone_test)
