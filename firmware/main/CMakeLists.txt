idf_component_register(
    SRCS 
        "midi_host_fw.c" 
        "class_driver.c" 
        "midi_translator.c" 
        "uart_driver.c" 

    INCLUDE_DIRS 
        "." 
        "../managed_components/sukuwc__grid_common/include"
)
