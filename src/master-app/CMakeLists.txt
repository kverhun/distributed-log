project(MasterApp)

add_executable(${PROJECT_NAME} master_app.cpp)
target_link_libraries(${PROJECT_NAME} Server)
