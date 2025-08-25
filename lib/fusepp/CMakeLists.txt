set(NAME fusepp)

add_library(${NAME} STATIC)

target_sources(${NAME} PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src/Fuse.cpp
)

target_include_directories(${NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${FUSE3_INCLUDE_DIRS}
)

target_link_libraries(${NAME}
    ${FUSE3_LIBRARIES}
)
