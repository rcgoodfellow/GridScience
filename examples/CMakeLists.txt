add_executable(ieee14 ieee14.cxx)
target_link_libraries(ieee14 gw_core ${MKL_LIBS})

link_directories(/usr/local/lib)
add_executable(ieee14_bson ieee14_bson.cxx)
include_directories(/usr/local/include)
target_link_libraries(ieee14_bson gw_core ${MKL_LIBS} mongoclient boost_system-mt)

