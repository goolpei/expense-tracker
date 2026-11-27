cmake_minimum_required(VERSION 3.20)
project(ExpenseTracker VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for IDE/Copilot IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------
# 1. SQLite3 Static Library (C)
# ---------------------------------------------------------
add_library(sqlite3_lib STATIC
    vendor/sqlite/sqlite3.c
)
target_include_directories(sqlite3_lib PUBLIC
    vendor/sqlite
)

# ---------------------------------------------------------
# 2. Dear ImGui + ImPlot Static Library (C++)
# ---------------------------------------------------------
add_library(imgui_implot_lib STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_impl_win32.cpp
    vendor/imgui/imgui_impl_dx11.cpp
    vendor/implot/implot.cpp
    vendor/implot/implot_demo.cpp
    vendor/implot/implot_items.cpp
)
target_include_directories(imgui_implot_lib PUBLIC
    vendor/imgui
    vendor/implot
)

# ---------------------------------------------------------
# 3. Main Windows Desktop Executable
# ---------------------------------------------------------
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
)

add_executable(${PROJECT_NAME} WIN32
    ${APP_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    sqlite3_lib
    imgui_implot_lib
    d3d11
    dxgi
    d3dcompiler
    dwmapi
    comdlg32
)

# Support WinMain entry point cleanly on both MSVC and MinGW-w64
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup"
    )
elseif(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
    
endif()

# ---------------------------------------------------------
# 4. Copy assets/ folder next to the compiled executable
# ---------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    COMMENT "Copying assets folder to build output directory..."
)