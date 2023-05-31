project(VectorTile)

add_library(VectorTile
mvt_utils.hpp
clara.hpp
mvt_reader.cpp
)


target_include_directories(
    VectorTile
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
 
)
target_link_libraries(
  VectorTile
  PRIVATE
)

install(TARGETS VectorTile
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/VectorTile
)
