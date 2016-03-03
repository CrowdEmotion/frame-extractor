#include <iostream>
#include "vld.h"
#include "FrameExtractor.h"

int main(int argc, char* argv[])
{
	using namespace std;

	string video, dfile, aus = "";

	if (argc == 1)
	{
		// video = "D:\\Projects\\Misc\\Video extractor\\data\\CVL1_test.mp4";
		video = "C:\\Users\\adg\\IdeaProjects\\Annotator\\temp\\rotosound_data\\39207\\39207.mp4";
		// dfile = "C:\\Users\\adg\\Documents\\GitHub\\ARIA-eMax\\eMax\\x64\\Release\\test.txt";
		dfile = "C:\\Users\\adg\\IdeaProjects\\Annotator\\temp\\rotosound_data\\39207\\39207.txt";
		aus = "C:\\Users\\adg\\Documents\\Visual Studio 2013\\Projects\\FrameExtractor\\x64\\Release\\aus.txt";
	}
	else if (argc == 2)
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
	else if (argc == 4)
	{
		video = argv[1];
		dfile = argv[2];
		aus = argv[3];
	}
	else
	{
		cout << "usage: " << argv[0] << " {video} or " << argv[0] << " {video} {dfile} or " << argv[0] << " {video} {efile} {ausfile}" << endl;
		return -1;
	}

	cout << "using video: " << video << " ; file: " << dfile << endl;

	FrameExtractor *extractor = new FrameExtractor;

	extractor->setDescriptorFile(dfile);
	extractor->setVideo(video);
	extractor->setAusFile(aus);

	int error = extractor->extract();
	switch (error)
	{
	case ERROR_NO_DESCRIPTOR:
		cerr << "no descriptor file specified" << endl;
		break;
	case ERROR_NO_VIDEO_FILE:
		cerr << "no video file specified" << endl;
		break;
	case ERROR_INVALID_DFILE:
		cerr << "invalid descriptor file" << endl;
		break;
	case ERROR_INVALID_VIDEO:
		cerr << "invalid video file" << endl;
		break;
	case ERROR_INVALID_AUS_F:
		cerr << "invalid aus file" << endl;
		break;
	}

	return error;
}

