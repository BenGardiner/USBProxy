#
# This file is part of USBProxy.
#

set(TOOL usb-mitm)

include_directories(${libusbproxy_SOURCE_DIR})
LIST(APPEND TOOLS_LINK_LIBS USBProxy)

add_executable(${TOOL} ${TOOL}.cpp)
set_source_files_properties(${TOOL}.cpp PROPERTIES COMPILE_FLAGS  "-std=c++11 -pthread -Wall")
target_link_libraries(${TOOL} ${TOOLS_LINK_LIBS})
install(TARGETS ${TOOL} RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(uinputme uinputme.c)
install(TARGETS uinputme RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(timerinputme timerinputme.c)
install(TARGETS timerinputme RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(evdev-usb-bridge evdev-usb-bridge.c)
set_target_properties(evdev-usb-bridge PROPERTIES COMPILE_FLAGS -O0)
install(TARGETS evdev-usb-bridge RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

add_executable(ignition-toggle ignition-toggle.c)
install(TARGETS ignition-toggle RUNTIME DESTINATION ${INSTALL_DEFAULT_BINDIR})

install(FILES
	${CMAKE_CURRENT_SOURCE_DIR}/buttonsme
	${CMAKE_CURRENT_SOURCE_DIR}/inject-reset-button
	${CMAKE_CURRENT_SOURCE_DIR}/send-a-udp
	${CMAKE_CURRENT_SOURCE_DIR}/inject-reset-button
	${CMAKE_CURRENT_SOURCE_DIR}/loopbacktest
	${CMAKE_CURRENT_SOURCE_DIR}/write-loopbacktest
	DESTINATION ${INSTALL_DEFAULT_BINDIR}
	COMPONENT pythonScripts
	PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
	)
