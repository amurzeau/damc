cmake_minimum_required(VERSION 3.1)

set(TARGET_NAME delayControl)

add_executable(${TARGET_NAME}
	delayControl.cpp
	DelayMeasure.cpp
	DelayMeasure.h
	WavLoader.cpp
	WavLoader.h
	PulseGenerator.cpp
	PulseGenerator.h
	PulseData.cpp
	PulseData.h
	ServerControl.cpp
	ServerControl.h
	ResamplingFilter.cpp
	ResamplingFilter.h
	ResamplingFilter_coefs.cpp
)
target_link_libraries(${TARGET_NAME} uv::uv damc_common)
target_compile_definitions(${TARGET_NAME} PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX)

target_link_libraries(${TARGET_NAME} JACK::jack)

install(TARGETS
	${TARGET_NAME}
	spdlog
	RUNTIME DESTINATION ./
	LIBRARY DESTINATION ./
)
install(FILES data/pulse.wav DESTINATION ./)
