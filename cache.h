#ifndef CACHE_H
#define CACHE_H
#include "swift.h"
#include <map>
#include <sstream>
#include <list>

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

namespace cache {
    struct  FileDescription;
    typedef std::map<swift::Sha1Hash,FileDescription> file_map_t;
    typedef std::list<swift::Sha1Hash> kickList_t;

    struct  MetaData{
    public:
        size_t      size;
        swift::tint lastused;
        // other stuff needed for calculation value of files
    };

    struct  FileDescription{
        uint64_t    file_number;
        uint32_t    open_count;
        kickList_t::iterator kickPointer;
        int         fd;
        MetaData    metaData;
    };

    class   Cache{
    public:
        Cache(const char* cache_dir,uint64_t cache_size);
        ~Cache();
        int     Open (const swift::Sha1Hash& hash);
        void    Close (int fd);
    private:
        std::string         getFileName(uint64_t file_number);
        void                kick();
        const char*         cache_dir;
        uint64_t            cache_size;
        uint64_t            cumulated_size;
        uint64_t            last_file_number;
        file_map_t          files;
        kickList_t          kickList;

        DISALLOW_COPY_AND_ASSIGN(Cache);
        friend struct compareLastUsed;
    };
}
#endif // CACHE_H
