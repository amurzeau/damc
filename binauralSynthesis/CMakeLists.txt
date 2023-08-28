cmake_minimum_required(VERSION 3.13)

add_executable(binauralSynthesis
	main.cpp
    BinauralSpacializer.cpp
    BinauralSpacializer.h
    Convolver.cpp
    Convolver.h
	DelayFilter.cpp
	DelayFilter.h
	DiscreteFourierTransform.cpp
	DiscreteFourierTransform.h
)
target_link_libraries(binauralSynthesis uv mysofa fftw3f)
target_compile_definitions(binauralSynthesis PRIVATE _USE_MATH_DEFINES _CRT_SECURE_NO_WARNINGS NOMINMAX JSON_SKIP_UNSUPPORTED_COMPILER_CHECK)

target_link_libraries(binauralSynthesis JACK::jack)

add_executable(testdft testdft.cpp DiscreteFourierTransform.cpp DiscreteFourierTransform.h)
target_link_libraries(testdft fftw3f)

install(TARGETS
	${TARGET_NAME}
	spdlog
	uv
	mysofa
	fftw3f
	RUNTIME DESTINATION ./
	LIBRARY DESTINATION ./
)
