add_library(config STATIC
        m_config.c
        m_config.h
        m_default.c
        m_extra.c
        m_variable.h
)

target_include_directories(config PRIVATE ${CMAKE_BINARY_DIR})
target_include_directories(config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(config PRIVATE cli common input SDL2::SDL2)
