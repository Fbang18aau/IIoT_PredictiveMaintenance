set(srcs "main.c"
        "board.c"
       
        "D_BMX160.c"
        "D_I2C.c")

idf_component_register(SRCS "BLEmesh_SensorServer.c" "${srcs}"
                    INCLUDE_DIRS  ".")
