#pragma once
#include "../mem/memory.h"
#include <filesystem>
#include <fstream>
#include <utility>

static inline std::unordered_map<std::string, std::string> mime_mapping {
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".txt", "text/plain"},
        {".csv", "text/csv"},

        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".webp", "image/webp"},
        {".ico", "image/x-icon"},
        {".svg", "image/svg+xml"},

        {".mp3", "audio/mpeg"},
        {".wav", "audio/wav"},
        {".ogg", "audio/ogg"},
        {".flac", "audio/flac"},
        {".aac", "audio/aac"},

        {".mp4", "video/mp4"},
        {".webm", "video/webm"},
        {".ogg", "video/ogg"},
        {".avi", "video/x-msvideo"},
        {".mov", "video/quicktime"},
        {".wmv", "video/x-ms-wmv"},
        {".flv", "video/x-flv"},
        {".mkv", "video/x-matroska"},

        {".zip", "application/zip"},
        {".tar", "application/x-tar"},
        {".gz", "application/gzip"},
        {".rar", "application/vnd.rar"},
        {".7z", "application/x-7z-compressed"},

        {".pdf", "application/pdf"},
        {".doc", "application/msword"},
        {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {".xls", "application/vnd.ms-excel"},
        {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        {".ppt", "application/vnd.ms-powerpoint"},
        {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},

        {".eot", "application/vnd.ms-fontobject"},
        {".ttf", "font/ttf"},
        {".otf", "font/otf"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},

        {".exe", "application/octet-stream"},
        {".bin", "application/octet-stream"},
        {".dll", "application/octet-stream"},
        {".iso", "application/octet-stream"},
        {".img", "application/octet-stream"},

        {".wasm", "application/wasm"},

        {".unknown", "application/octet-stream"}
};

namespace Nexus::IO {
    class ResourceLocator {
    public:
        struct Resource {
            Resource(std::string m) : mime(std::move(m)) {}
            Resource() = default;
            Nexus::Base::SharedPool<> data {1024};
            std::string mime;
        };
        static Nexus::Utils::MayFail<Resource> LocateResource(const std::string& request_path) {
            using namespace Nexus::Base;
            using namespace Nexus::Utils;
            std::string pathstr("static");
            pathstr.append(request_path);
            std::filesystem::path path(pathstr);
            if (!exists(path)) {
                return failed;
            }
            Resource res;
            if (mime_mapping.contains(path.extension().string())) res.mime = mime_mapping[path.extension().string()];
            else res.mime = "application/octet-stream";
            std::ifstream fs(path, std::ios::in | std::ios::binary);
            char buf[1024];
            while (fs) {
                fs.read(buf, 1024);
                if (fs.gcount() > 0) {
                    res.data.write(buf, fs.gcount());
                }
            }
            fs.close();
            return res;
        }
    };
}