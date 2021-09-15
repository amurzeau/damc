#pragma once

#include <jack/jack.h>
#include <spdlog/spdlog.h>

class JackUtils {
public:
	static void logJackStatus(spdlog::level::level_enum level, jack_status_t status);
};
