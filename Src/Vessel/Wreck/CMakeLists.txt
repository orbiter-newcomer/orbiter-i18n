# Copyright (c) 2026 orbiter-i18n contributors
# Licensed under the MIT License

set(CONFIG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Config)

add_library(Wreck SHARED
	Wreck.cpp
)

set_target_properties(Wreck
	PROPERTIES
	FOLDER Vessels
)

target_include_directories(Wreck
	PUBLIC ${ORBITER_SOURCE_SDK_INCLUDE_DIR}
)

target_link_libraries(Wreck
	${ORBITER_LIB}
	${ORBITER_SDK_LIB}
)

add_dependencies(Wreck
	${OrbiterTgt}
	Orbitersdk
)

add_custom_command(
	TARGET Wreck PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CONFIG_SOURCE_DIR}/ ${CMAKE_BINARY_DIR}/Config/Vessels
)

install(TARGETS Wreck
	RUNTIME
	DESTINATION ${ORBITER_INSTALL_MODULE_DIR}
)
install(DIRECTORY ${CONFIG_SOURCE_DIR}/
	DESTINATION ${ORBITER_INSTALL_ROOT_DIR}/Config/Vessels
)