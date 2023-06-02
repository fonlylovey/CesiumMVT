project(VectorTileReader)

add_library(VectorTileReader
VectorTileReader.hpp
mvt_reader.cpp
)


target_include_directories(
  VectorTileReader
    SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/vtzero/
        ${CMAKE_CURRENT_SOURCE_DIR}/include/protozero/
 
)
target_link_libraries(
  VectorTileReader
  PRIVATE
)

install(TARGETS VectorTileReader
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/VectorTileReader
)
