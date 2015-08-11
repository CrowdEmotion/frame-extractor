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
	double start, end, duration, mid;
	std::string type;
};

struct video_frame
{
	video_frame(cv::Mat img, double time, int64 id)
	{
		image = img;
		start_time = time;
		frame_id = id;
	}

	double start_time;
	int64 frame_id;
	cv::Mat image;
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

	void writeFrame(video_frame&, std::string& type, int digitCount);

	void writeWindow(std::list<video_frame>&, std::string& type, int digitCount);
};

template <class T>
class circular_list {
public:
	circular_list(int size)
	{
		max_size = size;
	}

	~circular_list() {  }

	void push(T element)
	{
		buffer.push_back(element);
		while (buffer.size() > max_size)
		{
			buffer.pop_front();
		}
	}

	void push(circular_list<T> elements)
	{
		buffer.insert(buffer.end(), elements.buffer.begin(), elements.buffer.end());

		while (buffer.size() > max_size)
		{
			buffer.pop_front();
		}
	}

	std::list<T> getList()
	{
		// make sure that a fresh copy is done
		std::list<T> temp{ begin(buffer), end(buffer) };

		return temp;
	}

	T getMid()
	{
		int mid = buffer.size() / 2;
		auto it = std::next(buffer.begin(), mid);
		return *it;
	}

private:
	std::list<T> buffer;
	int max_size;
};

#endif