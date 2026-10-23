// 工程根目录定位（header-only）。
// 依次尝试：--root=PATH -> VISION_ROOT -> 当前目录 -> 可执行文件所在目录往上找。
// 这样从任何工作目录都能跑（.app 启动时工作目录是 /）。
#pragma once
#include <cstdlib>
#include <string>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

namespace paths {

inline std::string exePath() {
#ifdef __APPLE__
    char buf[4096];
    uint32_t sz = sizeof(buf);
    if (_NSGetExecutablePath(buf, &sz) == 0) return std::string(buf);
#elif defined(__linux__)
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = 0;
        return std::string(buf);
    }
#endif
    return "";
}

inline bool hasConfig(const std::string& dir) {
    return !dir.empty() && access((dir + "/config/colors.yml").c_str(), R_OK) == 0;
}

inline std::string detectRoot() {
    if (const char* r = std::getenv("VISION_ROOT")) return r;
    if (hasConfig(".")) return ".";
    std::string dir = exePath();
    size_t slash = dir.find_last_of('/');
    if (slash != std::string::npos) dir = dir.substr(0, slash);
    for (int up = 0; up < 6 && !dir.empty(); up++) {  // build/ 或 .app/Contents/MacOS -> 工程根
        if (hasConfig(dir)) return dir;
        slash = dir.find_last_of('/');
        if (slash == std::string::npos || slash == 0) break;
        dir = dir.substr(0, slash);
    }
    return ".";
}

inline const std::string& root() {
    static const std::string r = detectRoot();
    return r;
}

// 把工程内的相对路径（config/... 、data/... 、out/...）拼成可用路径
inline std::string pathOf(const std::string& rel) { return root() + "/" + rel; }

} // namespace paths
