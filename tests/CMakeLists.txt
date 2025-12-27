add_executable(ReminderSchedulerTests
    core/test_reminder_scheduler.cpp
    ${CMAKE_SOURCE_DIR}/services/ReminderScheduler.cpp
)

target_include_directories(ReminderSchedulerTests PRIVATE
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/services
)

target_link_libraries(ReminderSchedulerTests PRIVATE Qt6::Test)

add_test(NAME ReminderSchedulerTests COMMAND ReminderSchedulerTests)

add_executable(PlantRepositoryTests
    data/test_plant_repository.cpp
    ${CMAKE_SOURCE_DIR}/data/PlantRepository.cpp
)

target_include_directories(PlantRepositoryTests PRIVATE
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/data
)

target_link_libraries(PlantRepositoryTests PRIVATE Qt6::Test Qt6::Sql)

add_test(NAME PlantRepositoryTests COMMAND PlantRepositoryTests)

add_executable(ReminderRepositoryTests
    data/test_reminder_repository.cpp
    ${CMAKE_SOURCE_DIR}/data/ReminderRepository.cpp
)

target_include_directories(ReminderRepositoryTests PRIVATE
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/data
)

target_link_libraries(ReminderRepositoryTests PRIVATE Qt6::Test Qt6::Sql)

add_test(NAME ReminderRepositoryTests COMMAND ReminderRepositoryTests)

# QML tests
qt_add_executable(QmlTests
    qml/qmltest_main.cpp
)

target_link_libraries(QmlTests PRIVATE Qt6::QuickTest)

add_test(NAME QmlTests COMMAND QmlTests)
set_tests_properties(QmlTests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/qml
)
