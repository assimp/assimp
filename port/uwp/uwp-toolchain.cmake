set(CMAKE_SYSTEM_NAME WindowsStore)
set(CMAKE_SYSTEM_VERSION 10.0)
add_compile_options(/ZW)			# Consume Windows Runtime
add_compile_options(/EHsc)			# C++ exceptions
add_definitions("-DWindowsStore -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE")