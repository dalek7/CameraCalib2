
#ifndef __DDUTIL_H__
#define __DDUTIL_H__

#include <memory>
#include <string>
#include <stdexcept>
#include <algorithm>

using namespace std;

/// https://stackoverflow.com/a/26221725
template<typename ... Args>
inline std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

#include <iostream>
#include <string>
#include <filesystem>
#include <unistd.h>


using std::filesystem::current_path;
inline static 
std::string exe_path()
{
    return current_path();
}

#ifdef _WINDOWS
#include <time.h>
#else
#include <sys/time.h>
#endif

inline static std::string get_timestring()
{

    std::string str1;
    // only for Windows
    #ifdef _WINDOWS
        SYSTEMTIME st;
        GetSystemTime(&st);
        str1 = string_format("%d%02d%02d_%02d%02d%02d_%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    #else
        time_t t = time(0);   // get time now
        struct tm * now = localtime( & t );
        struct timeval ts;
        gettimeofday(&ts,0);
        double tu = ts.tv_usec;
        char buf[255] = {0,};
        sprintf(buf, "%d%02d%02d_%02d%02d%02d_%.f",    now->tm_year + 1900,
                                                                now->tm_mon + 1,
                                                                now->tm_mday,
                                                                now->tm_hour,
                                                                now->tm_min,
                                                                now->tm_sec,
                                                                tu);

        str1 = std::string(buf);

    #endif





    return str1;

}

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
// http://stackoverflow.com/a/7430262
inline void mkdir_if_not_exist(std::string dir_name, mode_t mode=0755)
{
    struct stat st = {0};

    if (stat(dir_name.c_str(), &st) == -1)
    {
        mkdir(dir_name.c_str(), mode);
    }
    else
    {
        std::cout << "Directory " << dir_name << " already exists !" << std::endl;
    }
}

#include <vector>
// https://wiki.kldp.org/HOWTO/html/C++Programming-HOWTO/standard-string.html
inline static
void tokenize(const string& str,
					  std::vector<std::string>& tokens,
					  const string& delimiters)
{
	// 맨 첫 글자가 구분자인 경우 무시
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // 구분자가 아닌 첫 글자를 찾는다
    std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        // token을 찾았으니 vector에 추가한다
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // 구분자를 뛰어넘는다.  "not_of"에 주의하라
        lastPos = str.find_first_not_of(delimiters, pos);
        // 다음 구분자가 아닌 글자를 찾는다
        pos = str.find_first_of(delimiters, lastPos);
    }
}


// Returns a list of files in a directory (except the ones that begin with a dot)
// http://stackoverflow.com/a/1932861
#include <dirent.h>
#include <sys/stat.h>
#include <vector>

inline void get_files_in_directory(std::vector<string> &out, const std::string &directory)
{

	std::cout << directory.c_str() << std::endl;
	DIR *dir;
	class dirent *ent;
	class stat st;

	dir = opendir(directory.c_str());


	while ((ent = readdir(dir)) != NULL) {
		const string file_name = ent->d_name;
		const string full_file_name = directory + "/" + file_name;

		if (file_name[0] == '.')
		    continue;

		if (stat(full_file_name.c_str(), &st) == -1)
		    continue;

		const bool is_directory = (st.st_mode & S_IFDIR) != 0;

		if (is_directory)
		    continue;

		out.push_back(full_file_name);
	}

    closedir(dir);

    std::sort(out.begin(), out.end());

}

// case insensitive equal
inline static
bool is_equal_strings(const std::string& a, const std::string& b)
{
	size_t sz = a.size();
	if (b.size() != sz)
		return false;
	for (unsigned int i = 0; i < sz; ++i)
		if (tolower(a[i]) != tolower(b[i]))
			return false;
	return true;
}


inline static
int get_files_in_directory(std::vector<std::string> &out, const string &directory, const string &_extension)
{
	std::vector<std::string> fns;
	get_files_in_directory(fns, directory);
	std::vector<std::string> fn1;
	for (std::vector<string>::iterator it = fns.begin(); it != fns.end(); ++it)
	{
		vector<string> tokens;
		tokenize(*it, tokens, ".");
		string ext = tokens[tokens.size() - 1];

		if (is_equal_strings(ext, _extension))
		{
			fn1.push_back(*it);

		}
	}
	out = fn1;

	std::sort(out.begin(), out.end());

	return fn1.size();
}

#endif