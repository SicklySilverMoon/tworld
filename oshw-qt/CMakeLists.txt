add_library(oshw-qt STATIC "")
target_sources(oshw-qt PRIVATE
    ../generic/generic.h
    ../generic/_in.cpp
    ../generic/tile.c
    ../generic/timer.c
    oshwbind.h
    oshwbind.cpp
    CCMetaData.h
    CCMetaData.cpp
    TWApp.h
    TWApp.cpp
    TWDisplayWidget.h
    TWDisplayWidget.cpp
    TWMainWnd.h
    TWMainWnd.cpp
    TWMainWnd.ui
    TWProgressBar.h
    TWProgressBar.cpp
)

target_include_directories(oshw-qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/generic
)

target_link_libraries(oshw-qt PUBLIC
    ${SDL_LIBRARY}
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::Xml
    Qt::Multimedia
)
