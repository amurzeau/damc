cmake_minimum_required(VERSION 3.1)

add_executable(delayControl
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
	json.h
	../waveSendUDPJack/OutputInstance/ResamplingFilter.h
	../waveSendUDPJack/OutputInstance/ResamplingFilter.cpp
	../waveSendUDPJack/OutputInstance/ResamplingFilter_coefs.cpp
)
target_link_libraries(delayControl uv::uv)
target_compile_definitions(delayControl PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX JSON_SKIP_UNSUPPORTED_COMPILER_CHECK)

target_link_libraries(delayControl JACK::jack)

install(TARGETS delayControl
	RUNTIME DESTINATION ./
	LIBRARY DESTINATION ./
)
