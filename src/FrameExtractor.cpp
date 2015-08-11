#include "FrameExtractor.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include "Util.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <cmath>

static const int ADIACENT_FRAMES = 2;
static const int WINDOW_SIZE = ADIACENT_FRAMES + 1 + ADIACENT_FRAMES;

using namespace std;

bool compareFrame(const frame& first, const frame& second)
{
	return first.start <= second.start;
}

FrameExtractor::FrameExtractor() : file(nullptr), video(nullptr) 
{
	capture = make_unique<cv::VideoCapture>();
}

FrameExtractor::~FrameExtractor() {  }

void FrameExtractor::setDescriptorFile(string descriptorFile)
{
	file = make_unique<string>(descriptorFile);
}

void FrameExtractor::setVideo(string videoFile)
{
	video = make_unique<string>(videoFile);
}

int FrameExtractor::extract()
{
	if (file == nullptr)
	{
		return ERROR_NO_DESCRIPTOR;
	}
	if (video == nullptr)
	{
		return ERROR_NO_VIDEO_FILE;
	}
	if (readDFile() != 0)
	{
		return ERROR_INVALID_DFILE;
	}
	if (!capture->open(*video))
	{
		return ERROR_INVALID_VIDEO;
	}

	path = make_unique<string>(getParentPath(*video));
	name = make_unique<string>(getFileName(*video));

	int result = process();

	capture->release();

	return result;
}

int FrameExtractor::process()
{
	int i = 0;

	int64 frameCount = (int64)capture->get(CV_CAP_PROP_FRAME_COUNT);
	int countDigits = numDigits(frameCount);

	int64 lastTick = cv::getTickCount(), tickDiff = 1;
	double tickFrequency = cv::getTickFrequency();

	circular_list<video_frame> buffer(ADIACENT_FRAMES + 2);

	while (true)
	{
		double position = capture->get(CV_CAP_PROP_POS_MSEC) / 1000;
		int64 curFrame = (int64)capture->get(CV_CAP_PROP_POS_FRAMES);

		while (position > frames[i].end)
		{
			i++;
			if (i >= frames.size())
			{
				// exit the function
				goto stop;
			}
		}

		cv::Mat image;
		if (capture->read(image))
		{
			buffer.push(video_frame(image, position, curFrame));

			int64 tick = cv::getTickCount();
			tickDiff = tick - lastTick;
			lastTick = tick;
			fps = tickFrequency / tickDiff;

			if (frames[i].start < position && position < frames[i].end)
			{
				circular_list<video_frame> writeBuffer(WINDOW_SIZE);
				writeBuffer.push(buffer);

				double lastDist = abs(position - frames[i].mid);

				position = capture->get(CV_CAP_PROP_POS_MSEC) / 1000;
				curFrame = (int64)capture->get(CV_CAP_PROP_POS_FRAMES);

				double dist = abs(position - frames[i].mid);

				while (dist < lastDist)
				{
					position = capture->get(CV_CAP_PROP_POS_MSEC) / 1000;
					curFrame = (int64)capture->get(CV_CAP_PROP_POS_FRAMES);

					lastDist = dist;
					dist = abs(position - frames[i].mid);

					cv::Mat image_1;
					if (capture->read(image_1))
					{
						buffer.push(video_frame(image_1, position, curFrame));
						writeBuffer.push(video_frame(image_1, position, curFrame));
					}
					else
					{
						break;
					}
				}

				for (int j = 0; j < ADIACENT_FRAMES; j++)
				{
					position = capture->get(CV_CAP_PROP_POS_MSEC) / 1000;
					curFrame = (int64)capture->get(CV_CAP_PROP_POS_FRAMES);

					cv::Mat image_2;
					if (capture->read(image_2))
					{
						buffer.push(video_frame(image_2, position, curFrame));
						writeBuffer.push(video_frame(image_2, position, curFrame));
					}
					else
					{
						break;
					}
				}

				writeFrame(writeBuffer.getMid(), frames[i].type, countDigits);
				writeWindow(writeBuffer.getList(), frames[i].type, countDigits);

				i++;
				if (i >= frames.size())
				{
					// exit the function
					goto stop;
				}
			}

			int percent = (int)(curFrame * 100.0 / frameCount);

			cout << '\r' << percent << "% processed (" << setw(countDigits) << setfill('0') << curFrame << "/" << frameCount << ")";
		}
		else
		{
			cout << endl << "reached video end" << endl;
			return 0;
		}
	}

stop:
	cout << '\r' << "100% processed                     " << endl;
	return 0;
}

void FrameExtractor::writeFrame(video_frame &v_frame, string &type, int digitCount)
{
	ostringstream ss;
	ss << *path << "static" << separator() << *name << separator() << type;

	struct stat info;

	if (stat(ss.str().c_str(), &info) != 0)
	{
		// printf("cannot access %s\n", pathname);
		system(("mkdir \"" + ss.str() + "\"").c_str());
	}
	else if (info.st_mode & S_IFDIR)  // S_ISDIR() doesn't exist on my windows 
	{
		// printf("%s is a directory\n", pathname);
	}
	else
	{
		// printf("%s is no directory\n", pathname);
		system(("mkdir \"" + ss.str() + "\"").c_str());
	}

	ss << separator() << *name << "_" << setw(digitCount) << setfill('0') << v_frame.frame_id << ".png";

	// save file
	cv::imwrite(ss.str(), v_frame.image);
}

void FrameExtractor::writeWindow(list<video_frame> &window, string &type, int digitCount)
{
	int64 start_id = window.front().frame_id;
	int64 finish_id = window.back().frame_id;

	ostringstream ss;
	ss << *path << "sequence" << separator() << *name << separator() << type;

	struct stat info;

	if (stat(ss.str().c_str(), &info) != 0)
	{
		// printf("cannot access %s\n", pathname);
		system(("mkdir \"" + ss.str() + "\"").c_str());
	}
	else if (info.st_mode & S_IFDIR)  // S_ISDIR() doesn't exist on my windows 
	{
		// printf("%s is a directory\n", pathname);
	}
	else
	{
		// printf("%s is no directory\n", pathname);
		system(("mkdir \"" + ss.str() + "\"").c_str());
	}

	ss << separator() << *name; 
	ss << "_" << setw(digitCount) << setfill('0') << start_id;
	ss << "_" << setw(digitCount) << setfill('0') << finish_id;

	int index = 0;
	for (auto v_frame : window)
	{
		string file_path = ss.str() + "_" + to_string(index) + ".png";
		// save file
		cv::imwrite(file_path, v_frame.image);
		index++;
	}
}

int FrameExtractor::readDFile()
{
	ifstream infile(*file);
	string line;

	while (getline(infile, line))
	{
		istringstream iss(line);

		string buff;
		frame frame;

		if (!(iss >> frame.type >> buff)) 
		{
			infile.close();
			return 1;
		} // error

		while (!isdigit(buff.at(0)))
		{
			if (!(iss >> buff)) 
			{
				infile.close();
				return 2;
			} // error
		}

		if (!(iss >> frame.start >> buff >> frame.end >> buff >> frame.duration))
		{
			infile.close();
			return 3;
		} // error

		frame.mid = (frame.start + frame.end) / 2.0;

		// to upper case
		transform(frame.type.begin(), frame.type.end(), frame.type.begin(), ::toupper);

		frames.push_back(frame);
	}

	sort(frames.begin(), frames.end(), compareFrame);

	for (auto f : frames)
	{
		cout << f.type << " ; " << f.start << " ; " << f.end << endl;
	}

	infile.close();

	if (frames.size() == 0)
	{
		return 4;
	}

	return 0;
}