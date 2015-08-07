#include <iostream>
#include "vld.h"
#include "FrameExtractor.h"

int main(int argc, char* argv[])
{
	using namespace std;

	string video, dfile;

	if (argc == 2)
	{
		video = argv[1];

		int pos = (int)video.find_last_of('.');

		if (pos != string::npos)
		{
			dfile = video.substr(0, pos) + ".txt";
		}
		else
		{
			dfile = "";
		}
	}
	else if (argc == 3)
	{
		video = argv[1];
		dfile = argv[2];
	}
	else
	{
		cout << "usage: " << argv[0] << " {video} or " << argv[0] << " {video} {dfile}" << endl;
		return -1;
	}

	cout << "using video: " << video << " ; file: " << dfile << endl;

	FrameExtractor *extractor = new FrameExtractor;

	extractor->setDescriptorFile(dfile);
	extractor->setVideo(video);

	int error = extractor->extract();
	switch (error)
	{
	case ERROR_NO_DESCRIPTOR:
		cout << "no descriptor file specified" << endl;
		break;
	case ERROR_NO_VIDEO_FILE:
		cout << "no video file specified" << endl;
		break;
	case ERROR_INVALID_DFILE:
		cout << "invalid descriptor file" << endl;
		break;
	case ERROR_INVALID_VIDEO:
		cout << "invalid video file" << endl;
		break;
	}
	return error;
}

