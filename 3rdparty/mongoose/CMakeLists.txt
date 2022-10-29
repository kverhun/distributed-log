project(Mongoose)

add_library(${PROJECT_NAME} STATIC mongoose.c mongoose.h)
target_include_directories(${PROJECT_NAME} INTERFACE .)
