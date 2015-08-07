#ifndef __FrameExtractor__
#define __FrameExtractor__

#define ERROR_NO_DESCRIPTOR 1
#define ERROR_NO_VIDEO_FILE 2
#define ERROR_INVALID_DFILE 3
#define ERROR_INVALID_VIDEO 4

#include <string>
#include <memory>
#include <list>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\videoio\videoio.hpp>

struct frame
{
	double start, end, duration;
	std::string type;
};

class FrameExtractor
{
public:
	FrameExtractor();
	~FrameExtractor();

	void setDescriptorFile(std::string);

	void setVideo(std::string);

	int extract();

private:
	double fps;

	std::unique_ptr<std::string> file, video, path, name;
	std::unique_ptr<cv::VideoCapture> capture;

	std::vector<frame> frames;

	int process();

	int readDFile();
};

#endif