cmake_minimum_required(VERSION 3.22)

project(Yoink VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)

find_package(Qt6 6.6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Qml
    Quick
    QuickControls2
    Svg
)
find_package(KF6Config 6.6 REQUIRED)
find_package(KF6ColorScheme 6.6 REQUIRED)
find_package(KF6StatusNotifierItem 6.6 REQUIRED)
find_package(LayerShellQt REQUIRED)

qt_standard_project_setup()

set(YOINK_QML_FILES
    src/qml/Main.qml
    src/qml/EmptyState.qml
    src/qml/IconButton.qml
    src/qml/ItemMenu.qml
    src/qml/ItemRow.qml
    src/qml/MenuRow.qml
    src/qml/OptionChip.qml
    src/qml/SettingsPanel.qml
    src/qml/ShelfMenu.qml
    src/qml/TextButton.qml
    src/qml/Toast.qml
    src/qml/ToggleRow.qml
)

add_library(yoink_core STATIC
    src/core/AppSettings.cpp
    src/core/AppSettings.h
    src/core/MimeImport.cpp
    src/core/MimeImport.h
    src/core/ShelfItem.cpp
    src/core/ShelfItem.h
    src/core/ShelfLimits.h
    src/core/ShelfModel.cpp
    src/core/ShelfModel.h
    src/core/ShelfPlacement.cpp
    src/core/ShelfPlacement.h
    src/core/ShelfRepository.cpp
    src/core/ShelfRepository.h
    src/core/Theme.cpp
    src/core/Theme.h
)
target_include_directories(yoink_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(yoink_core PUBLIC
    Qt6::Core
    Qt6::Gui
    KF6::ConfigCore
    KF6::ColorScheme
)
target_compile_definitions(yoink_core PUBLIC YOINK_VERSION="${PROJECT_VERSION}")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(yoink_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

qt_add_executable(yoink
    src/main.cpp
    src/app/Autostart.cpp
    src/app/Autostart.h
    src/app/SingleInstance.cpp
    src/app/SingleInstance.h
    src/app/TrayIcon.cpp
    src/app/TrayIcon.h
    src/ui/DropSurface.cpp
    src/ui/DropSurface.h
    src/ui/ShelfController.cpp
    src/ui/ShelfController.h
    src/ui/ShelfSurface.cpp
    src/ui/ShelfSurface.h
    src/ui/YoinkImageProvider.cpp
    src/ui/YoinkImageProvider.h
)
target_link_libraries(yoink PRIVATE
    yoink_core
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    KF6::StatusNotifierItem
    LayerShellQt::Interface
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(yoink PRIVATE -Wall -Wextra -Wpedantic)
endif()

qt_add_resources(yoink "qml"
    PREFIX "/qml"
    BASE "src/qml"
    FILES ${YOINK_QML_FILES}
)
qt_add_resources(yoink "appicons"
    PREFIX "/icons"
    BASE "data/icons"
    FILES
        data/icons/org.yoink.Yoink.svg
        data/icons/org.yoink.Yoink-symbolic.svg
)

install(TARGETS yoink
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES data/org.yoink.Yoink.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(FILES data/org.yoink.Yoink.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
)
install(FILES
    data/icons/org.yoink.Yoink.svg
    data/icons/org.yoink.Yoink-symbolic.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)
install(FILES LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/yoink
)
