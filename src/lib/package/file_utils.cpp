#include "file_utils.hpp"
#include "../../common/defines.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <system_error>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define getcwd _getcwd
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif

namespace Lua {

// ===================================================================
// File Operations Implementation
// ===================================================================

bool FileUtils::fileExists(const Str& path) {
    if (path.empty()) {
        return false;
    }

    try {
        return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
    } catch (const std::filesystem::filesystem_error&) {
        // Fallback to traditional method
        std::ifstream file(path);
        return file.good();
    }
}

Str FileUtils::readFile(const Str& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    // Get file size
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read entire file
    Str content;
    content.resize(static_cast<size_t>(size));
    
    if (!file.read(&content[0], size)) {
        throw std::runtime_error("Cannot read file: " + path);
    }

    return content;
}

bool FileUtils::isDirectory(const Str& path) {
    if (path.empty()) {
        return false;
    }

    try {
        return std::filesystem::exists(path) && std::filesystem::is_directory(path);
    } catch (const std::filesystem::filesystem_error&) {
        // Fallback to platform-specific method
#ifdef _WIN32
        DWORD attrs = GetFileAttributesA(path.c_str());
        return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
        struct stat statbuf;
        return (stat(path.c_str(), &statbuf) == 0) && S_ISDIR(statbuf.st_mode);
#endif
    }
}

// ===================================================================
// Path Manipulation Implementation
// ===================================================================

Str FileUtils::joinPath(const Str& dir, const Str& file) {
    if (dir.empty()) {
        return file;
    }
    if (file.empty()) {
        return dir;
    }

    char sep = getPathSeparator();
    
    // Check if dir already ends with separator
    if (dir.back() == '/' || dir.back() == '\\') {
        return dir + file;
    }
    
    return dir + sep + file;
}

Str FileUtils::getDirectory(const Str& path) {
    if (path.empty()) {
        return "";
    }

    size_t pos = path.find_last_of("/\\");
    if (pos == Str::npos) {
        return ""; // No directory separator found
    }

    return path.substr(0, pos);
}

Str FileUtils::getFilename(const Str& path) {
    if (path.empty()) {
        return "";
    }

    size_t pos = path.find_last_of("/\\");
    if (pos == Str::npos) {
        return path; // No directory separator found
    }

    return path.substr(pos + 1);
}

Str FileUtils::getExtension(const Str& path) {
    Str filename = getFilename(path);
    size_t pos = filename.find_last_of('.');
    
    if (pos == Str::npos || pos == 0) {
        return ""; // No extension or hidden file
    }

    return filename.substr(pos);
}

Str FileUtils::removeExtension(const Str& path) {
    size_t pos = path.find_last_of('.');
    size_t sepPos = path.find_last_of("/\\");
    
    // Make sure the dot is in the filename, not in a directory name
    if (pos == Str::npos || (sepPos != Str::npos && pos < sepPos)) {
        return path; // No extension found
    }

    return path.substr(0, pos);
}

Str FileUtils::normalizePath(const Str& path) {
    if (path.empty()) {
        return path;
    }

    Str result = path;
    char sep = getPathSeparator();
    
    // Replace all separators with platform-specific separator
    std::replace(result.begin(), result.end(), 
                 (sep == '/') ? '\\' : '/', sep);
    
    return result;
}

// ===================================================================
// Module Search Operations Implementation
// ===================================================================

Str FileUtils::expandSearchPattern(const Str& pattern, const Str& modname) {
    if (pattern.empty() || modname.empty()) {
        return "";
    }

    // Convert module name to path (replace dots with slashes)
    Str modpath = moduleNameToPath(modname);
    
    // Replace ? with module path
    Str result = pattern;
    size_t pos = result.find('?');
    if (pos != Str::npos) {
        result.replace(pos, 1, modpath);
    }
    
    return normalizePath(result);
}

Str FileUtils::findModuleFile(const Str& modname, const Str& searchPath, 
                             const Str& sep, const Str& rep) {
    if (modname.empty() || searchPath.empty()) {
        return "";
    }

    // Convert module name to path
    Str modpath = moduleNameToPath(modname, sep, rep);
    
    // Split search path and try each pattern
    Vec<Str> patterns = splitSearchPath(searchPath);
    
    for (const Str& pattern : patterns) {
        if (pattern.empty()) {
            continue;
        }
        
        // Replace ? with module path
        Str filepath = pattern;
        size_t pos = filepath.find('?');
        if (pos != Str::npos) {
            filepath.replace(pos, 1, modpath);
        }
        
        // Normalize path and check if file exists
        filepath = normalizePath(filepath);
        if (fileExists(filepath)) {
            return filepath;
        }
    }
    
    return ""; // Not found
}

Vec<Str> FileUtils::splitSearchPath(const Str& searchPath) {
    if (searchPath.empty()) {
        return {};
    }

    char sep = getSearchPathSeparator();
    return split(searchPath, sep);
}

Str FileUtils::moduleNameToPath(const Str& modname, const Str& sep, const Str& rep) {
    if (modname.empty()) {
        return "";
    }

    return replaceAll(modname, sep, rep);
}

// ===================================================================
// Platform-Specific Utilities Implementation
// ===================================================================

char FileUtils::getPathSeparator() {
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

char FileUtils::getSearchPathSeparator() {
#ifdef _WIN32
    return ';';
#else
    return ':';
#endif
}

bool FileUtils::isAbsolutePath(const Str& path) {
    if (path.empty()) {
        return false;
    }

#ifdef _WIN32
    // Windows: C:\ or \\server\share
    return (path.length() >= 3 && path[1] == ':' && (path[2] == '\\' || path[2] == '/')) ||
           (path.length() >= 2 && path[0] == '\\' && path[1] == '\\');
#else
    // Unix: starts with /
    return path[0] == '/';
#endif
}

Str FileUtils::getCurrentDirectory() {
    try {
        return std::filesystem::current_path().string();
    } catch (const std::filesystem::filesystem_error&) {
        // Fallback to platform-specific method
        char buffer[1024];
        if (getcwd(buffer, sizeof(buffer)) != nullptr) {
            return Str(buffer);
        }
        throw std::runtime_error("Cannot get current directory");
    }
}

Str FileUtils::getLastError() {
#ifdef _WIN32
    DWORD error = GetLastError();
    if (error == 0) {
        return "No error";
    }

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, nullptr);

    Str message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
#else
    return std::strerror(errno);
#endif
}

// ===================================================================
// Internal Helper Functions Implementation
// ===================================================================

Str FileUtils::replaceAll(const Str& str, const Str& from, const Str& to) {
    if (str.empty() || from.empty()) {
        return str;
    }

    Str result = str;
    size_t pos = 0;

    while ((pos = result.find(from, pos)) != Str::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }

    return result;
}

Vec<Str> FileUtils::split(const Str& str, char delimiter) {
    Vec<Str> result;
    if (str.empty()) {
        return result;
    }

    std::stringstream ss(str);
    Str item;

    while (std::getline(ss, item, delimiter)) {
        item = trim(item);
        if (!item.empty()) {
            result.push_back(item);
        }
    }

    return result;
}

Str FileUtils::trim(const Str& str) {
    if (str.empty()) {
        return str;
    }

    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == Str::npos) {
        return ""; // String is all whitespace
    }

    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

} // namespace Lua
