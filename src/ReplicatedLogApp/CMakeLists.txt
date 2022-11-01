project(ReplicatedLog)

add_executable(${PROJECT_NAME} ReplicatedLog.cpp)
target_link_libraries(${PROJECT_NAME} Server)

find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} Threads::Threads)
