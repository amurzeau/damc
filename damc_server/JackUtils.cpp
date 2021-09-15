#include "JackUtils.h"
#include <Utils.h>
#include <map>
#include <string>

void JackUtils::logJackStatus(spdlog::level::level_enum level, jack_status_t status) {
	struct JackStatusDescription {
		uint32_t mask;
		const char* name;
		const char* description;
	};

	static const JackStatusDescription JACK_STATUSES[] = {
	    {0x01, "JackFailure", "Overall operation failed."},
	    {0x02, "JackInvalidOption", "The operation contained an invalid or unsupported option."},
	    {0x04, "JackNameNotUnique", "The desired client name was not unique."},
	    {0x08, "JackServerStarted", "The JACK server was started as a result of this operation."},
	    {0x10, "JackServerFailed", "Unable to connect to the JACK server."},
	    {0x20, "JackServerError", "Communication error with the JACK server."},
	    {0x40, "JackNoSuchClient", "Requested client does not exist."},
	    {0x80, "JackLoadFailure", "Unable to load internal client"},
	    {0x100, "JackInitFailure", "Unable to initialize client"},
	    {0x200, "JackShmFailure", "Unable to access shared memory"},
	    {0x400, "JackVersionError", "Client's protocol version does not match"},
	    {0x800, "JackBackendError", "Backend error"},
	    {0x1000, "JackClientZombie", "Client zombified failure"},
	};

	// Iterate each bit and check its value
	for(const JackStatusDescription& item : JACK_STATUSES) {
		if(status & item.mask) {
			SPDLOG_LOGGER_CALL(
			    spdlog::default_logger_raw(), level, " - {} ({}): {}", item.name, item.mask, item.description);
		}
	}
}
