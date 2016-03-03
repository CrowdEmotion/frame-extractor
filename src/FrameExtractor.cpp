#include "FrameExtractor.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include "Util.h"
#include <cmath>

static const int ADIACENT_FRAMES = 2;
static const int WINDOW_SIZE = ADIACENT_FRAMES + 1 + ADIACENT_FRAMES;

using namespace std;

bool compareFrame(const frame& first, const frame& second)
{
	return first.start < second.start;
}

bool compareTemporalFrames(const temporal_frame& first, const temporal_frame& second)
{
	return first.startFrame < second.startFrame;
}

double calculateMean(deque<double> list)
{
	auto temp = list;
	sort(temp.begin(), temp.end());
	if (temp.size() % 2 == 1) 
	{
		return temp[temp.size() / 2 + 1];
	}
	else
	{
		int mid = temp.size() / 2;
		return (temp[mid] + temp[mid + 1]) / 2;
	}
	return 0;
}

FrameExtractor::FrameExtractor() : file(nullptr), video(nullptr), useProgressMarker(false), auspath(nullptr)
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

void FrameExtractor::setUseProgressMarker(bool use)
{
	useProgressMarker = use;
}

void FrameExtractor::setAusFile(string aus)
{
	auspath = make_unique<string>(aus);
}

int FrameExtractor::extract()
{
	bool normalMode = auspath == nullptr || auspath->empty();
	if (file == nullptr)
	{
		return ERROR_NO_DESCRIPTOR;
	}
	if (video == nullptr)
	{
		return ERROR_NO_VIDEO_FILE;
	}
	if (normalMode)
	{
		if (readDFile() != 0)
		{
			return ERROR_INVALID_DFILE;
		}
	}
	else
	{
		if (readAusFile() != 0)
		{
			return ERROR_INVALID_AUS_F;
		}
		int result = readEFile();
		if (result != 0)
		{
			return ERROR_INVALID_DFILE;
		}
	}
	if (!capture->open(*video))
	{
		return ERROR_INVALID_VIDEO;
	}

	path = make_unique<string>(getParentPath(*video));
	name = make_unique<string>(getFileName(*video));

	cout << "Output path: " << *path << endl;

	int result;
	if (normalMode) 
	{
		result = process();
	}
	else
	{
		result = processEMax();
	}

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

int FrameExtractor::processEMax()
{
	int64 frameCount = (int64)capture->get(CV_CAP_PROP_FRAME_COUNT);
	int countDigits = numDigits(frameCount);

	double time = capture->get(CV_CAP_PROP_POS_MSEC) / 1000;
	int64 curFrame = (int64)capture->get(CV_CAP_PROP_POS_FRAMES);

	deque<temporal_frame> curFrames;

	cv::Mat image;
	while (capture->read(image))
	{
		if (temporalFrames.empty() && curFrames.empty())
		{
			break;
		}

		while (!temporalFrames.empty() && temporalFrames[0].startFrame == curFrame)
		{
			curFrames.push_back(temporalFrames[0]); 
			temporalFrames.pop_front();
		}

		auto &it = curFrames.begin();
		while (it != curFrames.end())
		{
			cv::Rect bbox = it->bboxs[0];
			int expandX = (int)(bbox.width * 0.5);
			int expandY = (int)(bbox.height * 0.5);
			bbox.x -= expandX;
			if (bbox.x < 0) 
			{
				bbox.x = 0;
			}
			bbox.y -= expandY;
			if (bbox.y < 0)
			{
				bbox.y = 0;
			}
			bbox.width += expandX * 2;
			if (bbox.x + bbox.width >= image.cols)
			{
				bbox.width = image.cols - bbox.x - 1;
			}
			bbox.height += expandY * 2;
			if (bbox.y + bbox.height >= image.rows)
			{
				bbox.height = image.rows - bbox.y - 1;
			}
			// take bounding box for all frames
			// it->bboxs.pop_front();

			it->frames.push_back(video_frame(cv::Mat(image, bbox).clone(), time, curFrame));

			if (it->frames.size() == 1)
			{
				it->startTime = time;
			}
			if (it->frames.size() == WINDOW_SIZE)
			{
				it->endTime = time;
				list<video_frame> temp{ begin(it->frames), end(it->frames) };

				writeWindow(temp, it->type, countDigits, &(*it));

				it = curFrames.erase(it);
			}
			else
			{
				it++;
			}
		}

		cout << ">>>>" << curFrame << "/" << frameCount << endl;

		time = capture->get(CV_CAP_PROP_POS_MSEC) / 1000;
		curFrame = (int64)capture->get(CV_CAP_PROP_POS_FRAMES);
	}

	cout << ">>>>" << frameCount << "/" << frameCount << endl;

	return 0;
}

void FrameExtractor::writeFrame(video_frame &v_frame, string &type, int digitCount)
{
	ostringstream ss;
	ss << *path << "static" << separator() << *name << separator() << type;

	mkdirIfRequired(ss.str());

	ss << separator() << *name << "_" << setw(digitCount) << setfill('0') << v_frame.frame_id << ".png";

	// save file
	cv::imwrite(ss.str(), v_frame.image);
}

void FrameExtractor::writeWindow(list<video_frame> &window, string &type, int digitCount, temporal_frame *temporalFrame)
{
	int64 start_id = window.front().frame_id;
	int64 finish_id = window.back().frame_id;

	ostringstream ss;
	ss << *path << "sequence" << separator() << *name << separator() << type;

	mkdirIfRequired(ss.str());

	ss << separator() << *name; 
	ss << "_" << setw(digitCount) << setfill('0') << start_id;
	ss << "_" << setw(digitCount) << setfill('0') << finish_id;

	int index = 0;
	for (auto &v_frame : window)
	{
		string file_path = ss.str() + "_" + to_string(index) + ".png";
		// save file
		cv::imwrite(file_path, v_frame.image);
		index++;
	}

	if (temporalFrame != 0)
	{
		ofstream outFile(ss.str() + ".txt", std::ofstream::trunc);

		outFile << "{ ";
		outFile << "\"v\":" << 1 << ", ";
		outFile << "\"faceId\":" << temporalFrame->faceId << ", ";
		outFile << "\"score\":" << temporalFrame->score << ", ";
		outFile << "\"startTime\":" << temporalFrame->startTime << ", ";
		outFile << "\"endTime\":" << temporalFrame->endTime << ", ";
		outFile << "\"startFrame\":" << temporalFrame->startFrame << ", ";
		outFile << "\"frameCount\":" << WINDOW_SIZE;
		outFile << " }";

		outFile.close();
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

	//if (frames.size() == 0)
	//{
	//	return 4;
	//}

	return 0;
}

int FrameExtractor::readEFile()
{
	int frameCount = 0;

	ifstream infile(*file);
	string line;

	while (getline(infile, line))
	{
		istringstream iss(line);

		string buff;
		int faceCount;
		emax_frame frame;

		// read the frame id and the number of faces, using a buffer for the comma
		if (!(iss >> frame.frameId >> buff >> faceCount >> buff))
		{
			infile.close();
			return 1;
		}
		for (int i = 0; i < faceCount; i++)
		{
			// read each face
			emax_face face;
			if (!(iss >> face.faceId >> buff))
			{
				infile.close();
				return 2;
			}
			// read the bounding boxes
			if (!(iss >> face.bbox.x >> buff >> face.bbox.y >> buff >> face.bbox.width >> buff >> face.bbox.height >> buff))
			{
				infile.close();
				return 3;
			}
			// read the results
			for (int j = 0; j < ausNames.size(); j++)
			{
				double val;
				if (iss >> val >> buff)
				{
					face.values.push_back(val);
				}
				else
				{
					infile.close();
					return 4;
				}
			}

			// put the the values in the 'data' structure
			const auto &it = data.find(face.faceId);
			if (it != data.end())
			{
				auto &e_data_vec = it->second;
				for (int j = 0; j < e_data_vec.size(); j++)
				{
					double val = face.values[j];
					emotion_data &e_data = e_data_vec[j];
					e_data.values.push_back(val);
					e_data.frameIds.push_back(frame.frameId);
					e_data.bboxs.push_back(face.bbox);
					if (val > 0)
					{
						e_data.average += val;
						e_data.count++;
						if (e_data.max < val)
						{
							e_data.max = val;
						}
					}
				}
			}
			else
			{
				emotion_data_vec e_data_vec;
				for (int j = 0; j < ausNames.size(); j++)
				{
					double val = face.values[j];
					emotion_data e_data;
					e_data.faceId = face.faceId;
					e_data.type = ausNames[j];
					e_data.values.push_back(val);
					e_data.frameIds.push_back(frame.frameId);
					e_data.bboxs.push_back(face.bbox);
					if (val > 0)
					{
						e_data.average = val;
						e_data.count = 1;
						e_data.max = val;
					}
					else
					{
						e_data.average = 0;
						e_data.count = 0;
						e_data.max = 0;
					}
					e_data_vec.push_back(e_data);
				}
				data[face.faceId] = e_data_vec;
			}

			frame.faces.push_back(face);
		}

		frameCount++;
		// raw_frames.push_back(frame);
	}

	infile.close();

	if (frameCount == 0)
	{
		return 5;
	}

	selectFrames();

	return 0;
}

void FrameExtractor::selectFrames()
{
	for (auto& it = data.begin(); it != data.end(); it++)
	{
		auto &e_data_vec = it->second;
		
		for (int j = 0; j < e_data_vec.size(); j++)
		{
			selectFrames(e_data_vec[j]);
		}
	}

	sort(temporalFrames.begin(), temporalFrames.end(), compareTemporalFrames);
}

void FrameExtractor::selectFrames(emotion_data& e_data)
{
	// calculate averages
	if (e_data.count == 0)
	{
		return;
	}
	e_data.average = e_data.average / e_data.count;

	int faceId = e_data.faceId;
	string &type = e_data.type;
	vector<int> &frameIds = e_data.frameIds;
	vector<double> &values = e_data.values;
	vector<cv::Rect> &bboxs = e_data.bboxs;
	double minVal = e_data.average < e_data.max / 2 ? e_data.max / 2 : e_data.average;

	deque<double> meanList;
	vector<temporal_frame> tFrames;
	for (int i = 1; i < values.size(); i++)
	{
		// check if the frames are one after each other
		if (frameIds[i] - frameIds[i - 1] != 1)
		{
			// start fresh
			meanList.clear();
		}
		double value = values[i];
		meanList.push_back(value);
		if (meanList.size() == WINDOW_SIZE)
		{
			// we have enough frames, calculate the mean
			double mean = calculateMean(meanList);
			meanList.pop_front();
			if (mean > e_data.max / 2)
			{
				// start position for this window
				int startpos = i - WINDOW_SIZE + 1;
				temporal_frame tFrame;
				tFrame.startFrame = frameIds[startpos];
				tFrame.type = type;
				tFrame.score = mean;
				tFrame.faceId = faceId;
				// copy the bounding boxes
				for (int j = startpos; j < startpos + WINDOW_SIZE; j++)
				{
					tFrame.bboxs.push_back(bboxs[j]);
				}
				tFrames.push_back(tFrame);
			}
		}
	}

	// handle special cases
	switch (tFrames.size())
	{
	case 0:
		return;
	case 1:
		temporalFrames.push_back(tFrames[0]);
		return;
	}
	
	temporal_frame* big = NULL;

	for (int i = 0; i < tFrames.size(); i++)
	{
		if (big == NULL)
		{
			big = &tFrames[i];
			continue;
		}
		temporal_frame &current = tFrames[i];
		if (abs(big->startFrame - current.startFrame) <= WINDOW_SIZE * 2)
		{
			if (big->score < current.score)
			{
				big = &current;
			}
		}
		else
		{
			temporalFrames.push_back(*big);
			big = &current;
		}
	}

	if (big != NULL)
	{
		// add the last element
		temporalFrames.push_back(*big);
	}
}

int FrameExtractor::readAusFile()
{
	ifstream infile(*auspath);
	string line;

	while (getline(infile, line))
	{
		istringstream iss(line);

		string name;

		while (iss >> name)
		{
			ausNames.push_back(name);
		}
	}

	infile.close();

	if (ausNames.size() == 0)
	{
		return 1;
	}
	
	return 0;
}