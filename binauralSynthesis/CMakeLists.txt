cmake_minimum_required(VERSION 2.8)

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

if(UNIX)
	target_link_libraries(binauralSynthesis jack)
else()
	target_link_libraries(binauralSynthesis "D:/Programmes/Musique/Jack/lib/libjack.lib")
	target_include_directories(binauralSynthesis PRIVATE "D:/Programmes/Musique/Jack/includes")
endif()

add_executable(testdft testdft.cpp DiscreteFourierTransform.cpp DiscreteFourierTransform.h)
target_link_libraries(testdft fftw3f)
