project(ReplicatedLog)

add_executable(${PROJECT_NAME} ReplicatedLog.cpp)
target_link_libraries(${PROJECT_NAME} Server)
