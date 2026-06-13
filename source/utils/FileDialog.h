#pragma once

#include <string>
#include <optional>

namespace rasterizer {

class FileDialog {
public:
    static std::optional<std::string> openFile(const std::string& title, const std::string& filter);
    
private:
    static std::optional<std::string> openFileMacOS(const std::string& title, const std::string& filter);
    static std::optional<std::string> openFileLinux(const std::string& title, const std::string& filter);
    static std::optional<std::string> openFileWindows(const std::string& title, const std::string& filter);
    
    static std::string trim(const std::string& str);
};

} 

