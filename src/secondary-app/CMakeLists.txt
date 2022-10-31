project(SecondaryApp)

add_executable(${PROJECT_NAME} secondary_app.cpp)
target_link_libraries(${PROJECT_NAME} Server)
