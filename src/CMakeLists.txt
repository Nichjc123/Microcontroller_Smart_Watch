#set(COMPONENT_SRCS "main.c")
#set(COMPONENT_ADD_INCLUDEDIRS "")

#register_component()

idf_component_register(SRCS "hid_device.c" "display_main.c" "main.c" "display_templates.c" 
                    INCLUDE_DIRS ".")
