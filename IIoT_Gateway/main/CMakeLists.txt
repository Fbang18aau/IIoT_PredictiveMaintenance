set(srcs "main.c"
        "BLEmesh_SensorClient.c"
        "D_AWS_MQTT.c"
        "D_GPIO.c" )

set(COMPONENT_ADD_INCLUDEDIRS
        "."
        "${CMAKE_CURRENT_LIST_DIR}"
        )

idf_component_register(SRCS "${srcs}"
        INCLUDE_DIRS  ${COMPONENT_ADD_INCLUDEDIRS}
       )
