find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(linuxcad
    main.cpp
    MainWindow.h
    MainWindow.cpp
    DrawingView.h
    DrawingView.cpp
    CommandLine.h
    CommandLine.cpp
    CommandDispatcher.h
    CommandDispatcher.cpp
    commands/DrawCommand.h
    commands/LineCommand.h
    commands/LineCommand.cpp
    commands/CircleCommand.h
    commands/CircleCommand.cpp
    commands/ArcCommand.h
    commands/ArcCommand.cpp
    commands/PolylineCommand.h
    commands/PolylineCommand.cpp
)

target_include_directories(linuxcad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(linuxcad PRIVATE lcad_core Qt6::Widgets Qt6::OpenGLWidgets)
