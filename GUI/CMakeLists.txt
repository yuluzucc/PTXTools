FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})

#message("building PTXViewer...")

QT4_WRAP_UI(PTXViewerUISrcs PTXViewerWidget.ui)
QT4_WRAP_CPP(PTXViewerMOCSrcs PTXViewerWidget.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

ADD_EXECUTABLE(PTXViewer PTXViewer.cpp PTXViewerWidget.cpp PointSelectionStyle3D.cpp
               ${PTXViewerMOCSrcs} ${PTXViewerUISrcs})
TARGET_LINK_LIBRARIES(PTXViewer ${PTXTools_libraries} ${QT_LIBRARIES})
INSTALL( TARGETS PTXViewer RUNTIME DESTINATION ${INSTALL_DIR} )
