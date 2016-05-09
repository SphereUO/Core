SET (TOOLCHAIN 1)

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-GNU-64.cmake.")
	SET(CMAKE_SYSTEM_NAME	"Windows"	PARENT_SCOPE)
	ENABLE_LANGUAGE(RC)

	SET (C_WARNING_OPTS "-Wall -Wextra -Wno-unknown-pragmas -Wno-switch  -Wno-error=unused-but-set-variable")
	SET (CXX_WARNING_OPTS "-Wall -Wextra -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-switch")
	SET (C_ARCH_OPTS "-march=x86-64 -m64")
	SET (CXX_ARCH_OPTS "-march=x86-64 -m64")
	SET (C_OPTS "-std=c11 -pthread -fno-omit-frame-pointer -fexceptions -fnon-call-exceptions")
	SET (CXX_OPTS "-std=c++11 -pthread -fno-omit-frame-pointer -fexceptions -fnon-call-exceptions -mno-ms-bitfields")
	# -mno-ms-bitfields is needed to fix structure packing; -s: strips debug info (remove it when debugging); -g: adds debug informations
	#	-pthread, -s and -g need to be added/removed also to/from linker flags!
	SET (C_SPECIAL "-s -O3 -fno-expensive-optimizations -pipe")
	SET (CXX_SPECIAL "-s -O3 -ffast-math -pipe")

	SET (CMAKE_RC_FLAGS	"--target=pe-x86-64"							PARENT_SCOPE)
	SET (CMAKE_C_FLAGS	"${C_WARNING_OPTS} ${C_ARCH_OPTS} ${C_OPTS} ${C_SPECIAL}"		PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CXX_WARNING_OPTS} ${CXX_ARCH_OPTS} ${CXX_OPTS} ${CXX_SPECIAL}"	PARENT_SCOPE)

	# Force dynamic linking but include into exe libstdc++ and libgcc.
	SET (CMAKE_EXE_LINKER_FLAGS	"-s -pthread -dynamic -static-libstdc++ -static-libgcc"		PARENT_SCOPE)
	LINK_DIRECTORIES ("${CMAKE_SOURCE_DIR}/common/mysql/lib/x86_64")

	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${CMAKE_BINARY_DIR}/bin64"	PARENT_SCOPE)
endfunction()

function (toolchain_exe_stuff)
	# Unix (MinGW) libs.
	FOREACH (LINK_LIB "mysql;ws2_32")
		TARGET_LINK_LIBRARIES ( spheresvr_release	${LINK_LIB} )
		TARGET_LINK_LIBRARIES ( spheresvr_debug		${LINK_LIB} )
		TARGET_LINK_LIBRARIES ( spheresvr_nightly	${LINK_LIB} )
	ENDFOREACH (LINK_LIB)

	# Defines.
	SET (COMMON_DEFS "_WIN32;_WIN64;_64BITS;_GITVERSION;_MTNETWORK;_EXCEPTIONS_DEBUG;_CRT_SECURE_NO_WARNINGS;_WINSOCK_DEPRECATED_NO_WARNINGS")
		# _WIN32: always defined, even on 64 bits. Keeping it for compatibility with external code and libraries.
		# _WIN64: 64 bits windows application. Keeping it for compatibility with external code and libraries.
		# _64BITS: 64 bits architecture
		# _EXCEPTIONS_DEBUG: Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
		#   on a running environment. Also it makes sphere more stable since exceptions are local.
		# _CRT_SECURE_NO_WARNINGS: Temporary setting to do not spam so much in the build proccess while we get rid of -W4 warnings and, after it, -Wall.
		# _WINSOCK_DEPRECATED_NO_WARNINGS: Removing warnings until the code gets updated or reviewed.
	FOREACH (DEF ${COMMON_DEFS})
		TARGET_COMPILE_DEFINITIONS ( spheresvr_release	PUBLIC ${DEF} )
		TARGET_COMPILE_DEFINITIONS ( spheresvr_debug	PUBLIC ${DEF} )
		TARGET_COMPILE_DEFINITIONS ( spheresvr_nightly	PUBLIC ${DEF} )
	ENDFOREACH (DEF)
	TARGET_COMPILE_DEFINITIONS ( spheresvr_release	PUBLIC THREAD_TRACK_CALLSTACK NDEBUG )
	TARGET_COMPILE_DEFINITIONS ( spheresvr_nightly	PUBLIC THREAD_TRACK_CALLSTACK NDEBUG _NIGHTLYBUILD )
	TARGET_COMPILE_DEFINITIONS ( spheresvr_debug	PUBLIC _DEBUG _PACKETDUMP _TESTEXCEPTION DEBUG_CRYPT_MSGS )
endfunction()