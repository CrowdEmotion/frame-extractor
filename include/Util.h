#ifndef __FrameExtractor_Util__
#define __FrameExtractor_Util__

#include <string>
#include <algorithm>
#include <stdint.h>
#include <sys/stat.h>

#define GET_FILE_TEST(path) std::cout << path << " : " << getFileName(path) << std::endl
#define GET_PARENT_TEST(path) std::cout << path << " : " << getParentPath(path) << std::endl

inline char separator()
{
#ifdef _WIN32
	return '\\';
#else
	return '/';
#endif
}

inline char oppositeSeparator()
{
#ifdef _WIN32
	return '/';
#else 
	return '\\';
#endif
}

static std::string getFileName(std::string path) 
{
	using namespace std;

	replace(path.begin(), path.end(), oppositeSeparator(), separator());

	int slash = (int)path.find_last_of(separator());
	int dot = (int)path.find_last_of('.');

	if (slash > dot || dot == string::npos)
	{
		return "output";
	}

	if (slash == string::npos)
	{
		return path.substr(0, dot);
	}
	else
	{
		return path.substr(slash + 1, dot - slash - 1);
	}
}

static std::string getParentPath(std::string path)
{
	using namespace std;

	replace(path.begin(), path.end(), oppositeSeparator(), separator());

	int slash = (int)path.find_last_of(separator());

	if (slash == string::npos)
	{
		return "";
	}

	return path.substr(0, slash) + separator();
}

static void mkdirIfRequired(const std::string &path)
{
	struct stat info;

	if (stat(path.c_str(), &info) != 0)
	{
		// printf("cannot access %s\n", pathname);
#if defined(_WIN32) || defined(_WIN64)
		system(("mkdir \"" + path + "\"").c_str());
#else
		system(("mkdir -p \"" + path + "\"").c_str());
#endif
	}
	else if (info.st_mode & S_IFDIR)  // S_ISDIR() doesn't exist on my windows 
	{
		// printf("%s is a directory\n", pathname);
	}
	else
	{
		// printf("%s is no directory\n", pathname);
#if defined(_WIN32) || defined(_WIN64)
		system(("mkdir \"" + path + "\"").c_str());
#else
		system(("mkdir -p \"" + path + "\"").c_str());
#endif
	}
}

// generic solution
template <class T>
static int numDigits(T number)
{
	int digits = 0;
	if (number < 0) digits = 1; // remove this line if '-' counts as a digit
	while (number) {
		number /= 10;
		digits++;
	}
	return digits;
}
// partial specialization optimization for 32-bit numbers
template<>
inline int numDigits(int32_t x)
{
	if (x == INTMAX_MIN) return 10 + 1;
	if (x < 0) return numDigits(-x) + 1;

	if (x >= 10000) {
		if (x >= 10000000) {
			if (x >= 100000000) {
				if (x >= 1000000000)
					return 10;
				return 9;
			}
			return 8;
		}
		if (x >= 100000) {
			if (x >= 1000000)
				return 7;
			return 6;
		}
		return 5;
	}
	if (x >= 100) {
		if (x >= 1000)
			return 4;
		return 3;
	}
	if (x >= 10)
		return 2;
	return 1;
}

// partial-specialization optimization for 8-bit numbers
template <>
inline int numDigits(char n)
{
	// if you have the time, replace this with a static initialization to avoid
	// the initial overhead & unnecessary branch
	static char x[256] = { 0 };
	if (x[0] == 0) {
		for (char c = 1; c != 0; c++)
			x[c] = numDigits((int32_t)c);
		x[0] = 1;
	}
	return x[n];
}
#ifndef _WIN32
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif

static void runTests()
{
	GET_FILE_TEST("test.xml");
	GET_FILE_TEST("../test.xml");
	GET_FILE_TEST("test");
	GET_FILE_TEST("C:\\folder\\test.xml");
	GET_FILE_TEST("C:\\folder");
	GET_FILE_TEST("folder.folder\\test.xml");
	GET_FILE_TEST("folder.folder\\test");
	GET_FILE_TEST("\\test.xml");

	GET_PARENT_TEST("test.xml");
	GET_PARENT_TEST("../test.xml");
	GET_PARENT_TEST("test");
	GET_PARENT_TEST("C:\\folder\\test.xml");
	GET_PARENT_TEST("C:\\folder");
	GET_PARENT_TEST("folder.folder\\test.xml");
	GET_PARENT_TEST("folder.folder\\test");
	GET_PARENT_TEST("\\test.xml");
	getchar();
}

#endif