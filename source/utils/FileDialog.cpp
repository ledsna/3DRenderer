#include "FileDialog.h"
#include <cstdio>
#include <cctype>
#include <algorithm>

#ifdef _WIN32
    #include <windows.h>
    #include <commdlg.h>
#endif

namespace rasterizer {

std::string FileDialog::trim(const std::string& str) {
    if (str.empty()) return str;
    
    // Find first non-whitespace
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    
    // Find last non-whitespace
    size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }
    
    return str.substr(start, end - start);
}

std::optional<std::string> FileDialog::openFile(const std::string& title, const std::string& filter) {
#if defined(_WIN32)
    return openFileWindows(title, filter);
#elif defined(__APPLE__)
    return openFileMacOS(title, filter);
#elif defined(__linux__)
    return openFileLinux(title, filter);
#else
    // Fallback: no dialog support
    return std::nullopt;
#endif
}

std::optional<std::string> FileDialog::openFileMacOS(const std::string& title, const std::string& filter) {
    // Build AppleScript with UTI filter for .obj files
    std::string filterClause;
    if (!filter.empty()) {
        // For .obj files, use public.geometry-definition-format UTI
        if (filter == "obj") {
            filterClause = " of type {\"public.geometry-definition-format\", \"obj\"}";
        } else {
            filterClause = " of type {\"" + filter + "\"}";
        }
    }
    
    std::string cmd = "osascript -e 'POSIX path of (choose file with prompt \"" 
                      + title + "\"" + filterClause + ")'";
    
    std::FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return std::nullopt;
    }
    
    std::string path;
    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), pipe)) {
        path += buffer;
    }
    
    int status = pclose(pipe);
    if (status != 0) {
        return std::nullopt;
    }
    
    path = trim(path);
    
    // Remove quotes if present
    if (path.size() >= 2 && 
        ((path.front() == '"' && path.back() == '"') || 
         (path.front() == '\'' && path.back() == '\''))) {
        path = path.substr(1, path.size() - 2);
    }
    
    return path.empty() ? std::nullopt : std::optional<std::string>(path);
}

std::optional<std::string> FileDialog::openFileLinux(const std::string& title, const std::string& filter) {
    std::string filterArg;
    if (!filter.empty()) {
        filterArg = " --file-filter=\"*." + filter + "\"";
    }
    
    std::string cmd = "zenity --file-selection --title=\"" + title + "\"" + filterArg;
    
    std::FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return std::nullopt;
    }
    
    std::string path;
    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), pipe)) {
        path += buffer;
    }
    
    int status = pclose(pipe);
    if (status != 0) {
        return std::nullopt;
    }
    
    path = trim(path);
    return path.empty() ? std::nullopt : std::optional<std::string>(path);
}

std::optional<std::string> FileDialog::openFileWindows([[maybe_unused]] const std::string& title, 
                                                        [[maybe_unused]] const std::string& filter) {
#ifdef _WIN32
    char filename[MAX_PATH] = "";
    
    // Build filter string (format: "Description\0*.ext\0\0")
    std::string filterStr;
    if (!filter.empty()) {
        filterStr = "OBJ Files (*." + filter + ")\0*." + filter + "\0All Files (*.*)\0*.*\0\0";
    } else {
        filterStr = "All Files (*.*)\0*.*\0\0";
    }
    
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filterStr.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(filename);
    }
    
    return std::nullopt;
#else
    return std::nullopt;
#endif
}

} 

