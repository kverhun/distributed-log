project(Server)

set(SOURCES Server.cpp Server.h)
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_link_libraries(${PROJECT_NAME} Mongoose)
target_include_directories(${PROJECT_NAME} INTERFACE .)
