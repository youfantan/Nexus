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
            Nexus::Base::FixedPool<> data;
            std::string mime;
            bool valid;
            std::atomic<int> hit {0};
            Resource(Nexus::Base::FixedPool<> data_, std::string  mime_, bool valid_) : data(std::move(data_)), mime(std::move(mime_)), valid(valid_) {}
            Resource(const Resource& r) : data(r.data), mime(r.mime), valid(r.valid), hit(r.hit.load()) {}
        };
    private:
        static std::mutex mtx_;
        static std::unordered_map<std::string, Resource> hotspot_map_;
    public:
        static uint64_t file_size(std::ifstream& fin)
        {
            auto b = fin.tellg();
            fin.seekg(0, std::ifstream::end);
            auto fos = fin.tellg();
            unsigned long long filesize = fos;
            fin.seekg(b, std::ifstream::beg);
            return filesize;
        }

        static Resource LocateResource(const std::string& request_path) {
            mtx_.lock();
            using namespace Nexus::Base;
            using namespace Nexus::Utils;
            if (hotspot_map_.contains(request_path)) {
                hotspot_map_.at(request_path).hit++;
                mtx_.unlock();
                return { hotspot_map_.at(request_path) };
            } else {
                std::string pathstr("static");
                pathstr.append(request_path);
                std::filesystem::path path(pathstr);
                if (!exists(path)) {
                    mtx_.unlock();
                    return { Nexus::Base::FixedPool(nullptr, 0), {}, false };
                }
                std::ifstream fs(path, std::ios::in | std::ios::binary);
                std::string mime;
                if (mime_mapping.contains(path.extension().string())) mime = mime_mapping[path.extension().string()];
                else mime = "application/octet-stream";
                auto sz = file_size(fs);
                char* mem = reinterpret_cast<char*>(malloc(sz));
                Resource res(FixedPool(mem, sz), mime, true);
                uint64_t readn = 0;
                while (fs) {
                    fs.read(mem + readn, 1024);
                    if (fs.gcount() > 0) {
                        readn += fs.gcount();
                    }
                }
                fs.close();
                hotspot_map_.emplace(request_path, res);
                mtx_.unlock();
                return res;
            }
        }
    };
    inline std::mutex ResourceLocator::mtx_{};
    inline std::unordered_map<std::string, ResourceLocator::Resource> ResourceLocator::hotspot_map_{};

}