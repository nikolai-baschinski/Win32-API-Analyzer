#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include <regex>
#include <io.h>
#include <fcntl.h>
#include <Windows.h>

using namespace std;

struct WIN32_API_Function {
	string name;
	unsigned int count = 0;
	WIN32_API_Function(string name, unsigned int count) : name(name), count(count) {};
};

void analisefileContent(const string& content, vector<WIN32_API_Function>& win32_api_functions)
{
	for (WIN32_API_Function& e : win32_api_functions) {
		size_t pos = content.find(e.name);
		while (pos != std::string::npos) {
			e.count++;
			pos = content.find(e.name, pos + e.name.length());
		}
	}
}

vector<wstring> findFilesInDirectory(wstring path)
{
	vector<wstring> files;
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	// Start iterating over the files in the path directory.
	hFind = ::FindFirstFileW(path.c_str(), &ffd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do
		{
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0) {

					wstring tempStr = path.substr(0, path.length() - 1) + ffd.cFileName + L"\\" + L"*";

					vector<wstring> tempFiles = findFilesInDirectory(tempStr);
					for (size_t j = 0; j < tempFiles.size(); j++) {
						files.push_back(tempFiles.at(j));
					}
				}
			} else {
				wstring tempStr = path.substr(0, path.length() - 1) + ffd.cFileName;
				files.push_back(tempStr);
				// wcout << L"File found: " << tempStr << endl;
			}
		} while (::FindNextFileW(hFind, &ffd) == TRUE);
		::FindClose(hFind);
	} else {
		wcerr << L"Failed to find path: " << path << endl;
		exit(3);
	}

	return files;
}

int wmain(int argc, wchar_t* argv[])
{
	int modeMemory = _setmode(_fileno(stdout), _O_U16TEXT);
	wstring pathToAnalize;

	// if there is no parameter, analyse the current directory recursively
	const DWORD BUFFER_SIZE = 256;
	WCHAR lpCurrentWorkingDirectory[BUFFER_SIZE];
	if (argc == 1) {
		DWORD rv = GetCurrentDirectoryW(BUFFER_SIZE, lpCurrentWorkingDirectory);
		if (rv == 0) {
			wcout << "Error occured. Error code: " << GetLastError() << endl;
			exit(1);
		} else if (rv > BUFFER_SIZE) {
			wcout << "Error occured. Current working directory longer than " << BUFFER_SIZE << " bytes." << endl;
			exit(2);
		} else {
			pathToAnalize = lpCurrentWorkingDirectory;
			pathToAnalize.append(L"\\*");
			wcout << "Project path to analyze: " << pathToAnalize << endl;
		}
	}
	if (argc >= 2) {
		pathToAnalize = argv[1];
		// case 1: path*
		if (pathToAnalize[pathToAnalize.size() - 1] == '*' && pathToAnalize[pathToAnalize.size() - 2] != '\\') {
			pathToAnalize.insert(pathToAnalize.size() - 1, L"\\");
		}
		// case 2: path\ 
		if (pathToAnalize[pathToAnalize.size() - 1] == '\\') {
			pathToAnalize.append(L"*");
		}
		// case 3: path
		if (pathToAnalize[pathToAnalize.size() - 1] != '*' && pathToAnalize[pathToAnalize.size() - 2] != '\\') {
			pathToAnalize.append(L"\\*");
		}
		wcout << "Project path to analyze: " << pathToAnalize << endl;
	}

	vector<wstring> filesSource;
	vector<wstring> filesAll = findFilesInDirectory(pathToAnalize);

	for (size_t i = 0; i < filesAll.size(); i++) {
		transform(filesAll.at(i).begin(), filesAll.at(i).end(), filesAll.at(i).begin(), ::tolower);
		wstring tempStr = filesAll.at(i);
		size_t length = tempStr.size();

		size_t found = tempStr.find(L".cpp");
		if (found != string::npos && found + 4 == length) {
			filesSource.push_back(tempStr);
			// wcout << filesAll.at(i) << endl;
		}

		found = tempStr.find(L".c");
		if (found != string::npos && found + 2 == length) {
			filesSource.push_back(tempStr);
			// wcout << filesAll.at(i) << endl;
		}

		found = tempStr.find(L".h");
		if (found != string::npos && found + 2 == length) {
			filesSource.push_back(tempStr);
			// wcout << filesAll.at(i) << endl;
		}
	}

	wcout << L"The folder contains " << filesAll.size() << L" files" << endl;
	wcout << L"The folder contains " << filesSource.size() << L" c/cpp/h files" << endl;

	// read the file with WIN32-API functions
	vector<WIN32_API_Function> Win32_API_functions;
	Win32_API_functions.reserve(2300);
	string fileContent;
	const wchar_t* Win32_API_filename = L"Win32-API-functions.txt";
	ifstream myfile(Win32_API_filename);

	if (myfile.is_open()) {
		string line;
		while (getline(myfile, line)) {
			Win32_API_functions.emplace_back(WIN32_API_Function(line, 0));
		}
		myfile.close();
		wcout << Win32_API_functions.size() << L" functions found in file " << Win32_API_filename << endl;
	} else {
		wcout << L"Unable to open file " << Win32_API_filename << L" in " << lpCurrentWorkingDirectory << endl;
	}

	// search the Win32-API-functions in the files
	for (size_t i = 0; i < filesSource.size(); i++) {
		string fileContent;
		ifstream myfile(filesSource.at(i));

		if (myfile.is_open()) {

			wcout << L"Analysing file number " << i + 1 << "\r";

			ostringstream buffer;
			buffer << myfile.rdbuf();
			analisefileContent(buffer.str(), Win32_API_functions);

			myfile.close();

		} else {
			wcout << L"Unable to open file" << filesSource.at(i) << endl;
		}
	}

	(void)_setmode(_fileno(stdout), modeMemory);

	cout << endl << endl ;
	for (WIN32_API_Function e : Win32_API_functions) {
		if (e.count != 0)
			cout << e.name << " : " << e.count << endl;
	}

	return 0;
}