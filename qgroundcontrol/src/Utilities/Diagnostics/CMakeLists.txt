add_library(QGCDiagnostics INTERFACE)

target_sources(QGCDiagnostics
    INTERFACE
        EdgeTriggeredCounter.h
)

target_include_directories(QGCDiagnostics
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCDiagnostics)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/SystemResourceMonitor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/SystemResourceMonitor.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/ScriptRunner.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ScriptRunner.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/AIChatService.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AIChatService.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/AIVehicleTools.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AIVehicleTools.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/AISettingsTools.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AISettingsTools.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/AIParameterTools.h
        ${CMAKE_CURRENT_SOURCE_DIR}/AIParameterTools.cc
)
