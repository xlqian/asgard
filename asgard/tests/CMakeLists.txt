
#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_executable(direct_path_response_builder_test direct_path_response_builder_test.cpp)
target_link_libraries(direct_path_response_builder_test protobuf ${Boost_LIBRARIES} libasgard valhalla z curl)
ADD_BOOST_TEST(direct_path_response_builder_test)

add_executable(util_test util_test.cpp)
target_link_libraries(util_test ${Boost_LIBRARIES} libasgard valhalla)
ADD_BOOST_TEST(util_test)

#add_executable(projector_test projector_test.cpp)
#target_link_libraries(projector_test ${Boost_LIBRARIES} protobuf boost_regex libasgard valhalla z curl)
#ADD_BOOST_TEST(projector_test)

add_executable(benchmark_projector_cache benchmark_projector_cache.cpp ${CMAKE_SOURCE_DIR}/utils/timer.cpp)
target_link_libraries(benchmark_projector_cache ${Boost_LIBRARIES} libasgard valhalla boost_program_options protobuf boost_regex profiler z curl)
