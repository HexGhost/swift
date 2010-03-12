#include "cache.h"
#include <sstream>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <assert.h>

namespace cache {

    struct compareLastUsed{
        Cache & cache;
        compareLastUsed(Cache & cache):cache(cache){
        }
        bool operator() (swift::Sha1Hash a, swift::Sha1Hash b) const{
            return cache.files[a].metaData.lastused < cache.files[b].metaData.lastused;
        }
    };

    FileDescription makeFileDescription(
            uint64_t file_number, uint32_t open_count, int fd, size_t size, swift::tint lastused){
        FileDescription fileDescription;
        fileDescription.file_number = file_number;
        fileDescription.open_count = open_count;
        fileDescription.fd = fd;
        fileDescription.metaData.size = size;
        fileDescription.metaData.lastused = lastused;
        return fileDescription;
    }

    Cache::Cache(const char* cache_dir,uint64_t cache_size):
            cache_dir(cache_dir),cache_size(cache_size),last_file_number(0),cumulated_size(0){
        mkdir(cache_dir, 0777);
        std::ifstream index(std::string(cache_dir).append("/index").c_str());
        if(index.is_open()){
            std::string tmp;
            std::getline(index, tmp);   //skip comment
            index >> last_file_number;
            while(index>>tmp, index.good()){
                swift::Sha1Hash hash(true, tmp.c_str());
                assert(hash != swift::Sha1Hash::ZERO);
                FileDescription fileDescription;
                index >> fileDescription.file_number >> fileDescription.metaData.lastused;
                fileDescription.fd = swift::Open(getFileName(fileDescription.file_number).c_str(), hash);
                if(fileDescription.fd > 0){
                    if(swift::Size(fileDescription.fd) &&
                       swift::Size(fileDescription.fd) == swift::Complete(fileDescription.fd)){
                        kickList.push_back(hash);
                        fileDescription.kickPointer = --kickList.end();
                        fileDescription.metaData.size = swift::Size(fileDescription.fd);
                        fileDescription.open_count = 0;
                        file_map_t::iterator it = files.insert(std::pair<swift::Sha1Hash,FileDescription>(hash,fileDescription)).first;
                        cumulated_size+=fileDescription.metaData.size;
                    }else{
                        swift::Close(fileDescription.fd);
                        std::string fileName = getFileName(fileDescription.file_number);
                        unlink(fileName.c_str());
                        unlink(fileName.append(".mhash").c_str());
                    }
                }
            }
            std::cout << (--kickList.end())->hex() <<std::endl;
//            for (kickList_t::iterator it = kickList.begin(); it != kickList.end(); it++)
//                std::cout << it->hex() << " " << files[*it].file_number << " " << files[*it].metaData.lastused << " " << (files[*it].kickPointer->hex()) << std::endl;
            kickList.sort(compareLastUsed(*this));
            for (kickList_t::iterator it = kickList.begin(); it != kickList.end(); it++)
                std::cout << it->hex() << " " << files[*it].file_number << " " << files[*it].metaData.lastused << std::endl;
            index.close();
        }
        std::cout << cumulated_size << std::endl;
    }

    Cache::~Cache(){
        std::ofstream index(std::string(cache_dir).append("/index").c_str());
        assert(index.is_open());
        index << "//Hash\tfile_number\tlastused_time\n";
        index << last_file_number << "\n";
        for(file_map_t::iterator iter = files.begin(); iter != files.end(); ++iter){
            index << iter->first.hex() << " " << iter->second.file_number << " " << iter->second.metaData.lastused << "\n";
            swift::Close(iter->second.fd);
        }
        index.close();
    }

    void Cache::Close (int fd){
        file_map_t::iterator res = files.find(swift::RootMerkleHash(fd));
        assert(res != files.end());
        res -> second.metaData.lastused = swift::Datagram::Time();
        res -> second.open_count--;
        std::cout << res -> second.file_number << " " << res -> second.open_count << std::endl;
        if(!res -> second.open_count){
            cumulated_size -= res -> second.metaData.size;
            res -> second.metaData.size = swift::Size(res -> second.fd);
            if(res -> second.metaData.size &&
               res -> second.metaData.size == swift::Complete(res -> second.fd)){
                kickList.push_back(res -> first);
                res -> second.kickPointer = --kickList.end();
                cumulated_size += res -> second.metaData.size;
                kick();
            }else{
                swift::Close(res -> second.fd);
                std::string fileName = getFileName(res -> second.file_number);
                unlink(fileName.c_str());
                unlink(fileName.append(".mhash").c_str());
                files.erase(res);
            }

            for (kickList_t::iterator it = kickList.begin(); it != kickList.end(); it++)
                std::cout << it->hex() << " " << files[*it].file_number << " " << files[*it].metaData.lastused << std::endl;
        }
    }

    int Cache::Open (const swift::Sha1Hash& hash){
        assert(hash != swift::Sha1Hash::ZERO);
        file_map_t::iterator res = files.find(hash);
        if(res != files.end()){
            if(!res -> second.open_count++)
                kickList.erase(res -> second.kickPointer);
            res -> second.metaData.lastused = swift::Datagram::Time();
            return res -> second.fd;
        }

        struct stat st;
        while(0 == stat(getFileName(++last_file_number).c_str(), &st));
        assert(errno == ENOENT);

        FileDescription fileDescription;
        fileDescription.fd = swift::Open(getFileName(last_file_number).c_str(), hash);
        fileDescription.file_number = last_file_number;
        fileDescription.open_count = 1;
        fileDescription.metaData.lastused = swift::Datagram::Time();
        fileDescription.metaData.size = 0;
        fileDescription.kickPointer = kickList.end();
        files.insert(std::pair<swift::Sha1Hash,FileDescription>(hash,fileDescription));

        return fileDescription.fd;
    }

    void Cache::kick(){
        std::cout << "cumul size " << cumulated_size << std::endl;
        for(kickList_t::iterator it = kickList.begin(); cumulated_size > cache_size && it != kickList.end(); it=kickList.erase(it)){
            file_map_t::iterator res = files.find(*it);
            assert(res != files.end());
            swift::Close(res -> second.fd);
            std::cout << "kicked " << res -> second.file_number <<std::endl;
            std::string fileName = getFileName(res -> second.file_number);
            unlink(fileName.c_str());
            unlink(fileName.append(".mhash").c_str());
            cumulated_size -= res -> second.metaData.size;
            files.erase(res);
        }
    }

    std::string Cache::getFileName(uint64_t file_number){
        std::stringstream filename;
        filename << cache_dir << "/cache" << file_number;
        return filename.str();
    }
}
