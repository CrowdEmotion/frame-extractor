#ifndef __FrameExtractor__
#define __FrameExtractor__

#define ERROR_NO_DESCRIPTOR 1
#define ERROR_NO_VIDEO_FILE 2
#define ERROR_INVALID_DFILE 3
#define ERROR_INVALID_VIDEO 4
#define ERROR_INVALID_AUS_F 5

#include <string>
#include <memory>
#include <list>
#include <map>
#include <deque>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\videoio\videoio.hpp>

struct emotion_data;
typedef std::vector<emotion_data> emotion_data_vec;

struct frame
{
	double start, end, duration, mid;
	std::string type;
};

struct video_frame
{
	video_frame(cv::Mat &img, double time, int64 id)
	{
		image = img;
		start_time = time;
		frame_id = id;
	}

	double start_time;
	int64 frame_id;
	cv::Mat image;
};

struct emotion_data
{
	int faceId;
	std::string type;
	std::vector<int> frameIds;
	std::vector<double> values;
	std::vector<cv::Rect> bboxs;
	double average, max;
	int count;
};

struct emax_face
{
	int faceId, frameId;
	cv::Rect bbox;
	std::vector<double> values;
};

struct emax_frame
{
	int frameId;
	std::vector<emax_face> faces;
};

struct temporal_frame
{
	std::string type;
	int startFrame, faceId;
	std::deque<cv::Rect> bboxs;
	double score;
	std::vector<video_frame> frames;
	double startTime;
	double endTime;
};

class FrameExtractor
{
public:
	FrameExtractor();
	~FrameExtractor();

	void setDescriptorFile(std::string);

	void setVideo(std::string);

	void setAusFile(std::string);

	void setUseProgressMarker(bool);

	int extract();

private:
	double fps;

	std::unique_ptr<std::string> file, video, path, name, auspath;
	std::unique_ptr<cv::VideoCapture> capture;

	std::vector<frame> frames;

	std::deque<temporal_frame> temporalFrames;

	std::vector<std::string> ausNames;

	std::map<int, emotion_data_vec> data;

	bool useProgressMarker;

	int process();

	int processEMax();

	int readDFile();

	int readEFile();

	int readAusFile();

	void selectFrames();

	void selectFrames(emotion_data& e_data);

	void writeFrame(video_frame&, std::string& type, int digitCount);

	void writeWindow(std::list<video_frame>&, std::string& type, int digitCount, temporal_frame *temporalFrame = NULL);
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