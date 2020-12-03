#include <stdio.h>
#include <string>
#include <vector>

#include <uv.h>

#include <jack/jack.h>
#include <mysofa.h>

#include "BinauralSpacializer.h"

/*
 * Postprocessing of SOFA HRTF :
 *  - Low frequency extension (http://audiogroup.web.th-koeln.de/FILES/AIA-DAGA2013_HRIRs.pdf)
 *  - flat group delay (http://audiogroup.web.th-koeln.de/FILES/AIA-DAGA2013_HRIRs.pdf)
 *  - normalized gain over all HRTF
 *  - diffuse field compensation
 */

void onTtyAllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = new char(suggested_size);
	buf->len = suggested_size;
}
void onTtyRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	BinauralSpacializer* delayMeasure = (BinauralSpacializer*) stream->data;
	free(buf->base);

	printf("Stopping\n");

	delayMeasure->stop();

	uv_read_stop(stream);
}

int main(int argc, char* argv[]) {
	BinauralSpacializer binauralSpacializer;
	uv_tty_t ttyRead;
	std::string speakerPlacement = "7.1";
	const char* filename = "RIEC_hrir_subject_001.sofa";

	if(argc >= 2)
		speakerPlacement = argv[1];

	if(speakerPlacement == "3D7.1")
		filename = "HRIR_FULL2DEG.sofa";

	if(argc >= 3)
		filename = argv[2];

	uv_tty_init(uv_default_loop(), &ttyRead, 0, 1);
	ttyRead.data = &binauralSpacializer;
	uv_read_start((uv_stream_t*) &ttyRead, &onTtyAllocBuffer, &onTtyRead);

	printf("Running binaural spacializer with speaker placement %s and HRTFs file %s\n",
	       speakerPlacement.c_str(),
	       filename);

	binauralSpacializer.start(filename, speakerPlacement);

	printf("Press enter to stop\n");

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	return 0;
}
