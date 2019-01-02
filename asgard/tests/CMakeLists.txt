
#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

add_executable(direct_path_response_builder_test direct_path_response_builder_test.cpp)
target_link_libraries(direct_path_response_builder_test protobuf ${Boost_LIBRARIES} libasgard valhalla)
ADD_BOOST_TEST(direct_path_response_builder_test)
