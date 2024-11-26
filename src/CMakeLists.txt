

add_executable(testSync testSync.cpp
    camera/camThread.cpp
    webapi/cameraController.cpp
    webapi/webApp.cpp
    )
target_link_libraries(testSync ${LIBS})