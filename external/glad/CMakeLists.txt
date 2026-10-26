# glad — OpenGL 4.0 core loader (сгенерирован glad 0.1.36)
add_library(glad STATIC src/glad.c)
add_library(glad::glad ALIAS glad)

# PUBLIC: все, кто линкуется с glad, автоматически получают его include-директорию
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# glad грузит функции GL через dlopen/dlsym
target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})

# PIC — чтобы glad можно было линковать и в .so
set_target_properties(glad PROPERTIES
    C_STANDARD 11
    POSITION_INDEPENDENT_CODE ON
)
