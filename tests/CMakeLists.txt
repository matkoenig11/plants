add_executable(ReminderSchedulerTests
    core/test_reminder_scheduler.cpp
)

target_link_libraries(ReminderSchedulerTests PRIVATE PlantJournalBackend Qt6::Test)

add_test(NAME ReminderSchedulerTests COMMAND ReminderSchedulerTests)

add_executable(PlantRepositoryTests
    data/test_plant_repository.cpp
)

target_link_libraries(PlantRepositoryTests PRIVATE PlantJournalBackend Qt6::Test Qt6::Sql)

add_test(NAME PlantRepositoryTests COMMAND PlantRepositoryTests)

add_executable(ReminderRepositoryTests
    data/test_reminder_repository.cpp
)

target_link_libraries(ReminderRepositoryTests PRIVATE PlantJournalBackend Qt6::Test Qt6::Sql)

add_test(NAME ReminderRepositoryTests COMMAND ReminderRepositoryTests)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(NAME SeedPopulationPython
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/check_seed.py ${CMAKE_SOURCE_DIR}
)

# QML tests
qt_add_executable(QmlTests
    qml/qmltest_main.cpp
)

target_link_libraries(QmlTests PRIVATE Qt6::QuickTest)

add_test(NAME QmlTests COMMAND QmlTests)
set_tests_properties(QmlTests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests/qml
)
