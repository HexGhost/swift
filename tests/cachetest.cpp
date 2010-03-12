//#include <cstdio>
#include <gtest/gtest.h>
#include "cache.h"

using namespace swift;
using namespace cache;

TEST(CacheTest,Creation) {


    srand ( time(NULL) );
    unlink("doc/sofi-copy.jpg");
    struct stat st;
    ASSERT_EQ(0,stat("doc/sofi.jpg", &st));
    int size = st.st_size;//, sizek = (st.st_size>>10) + (st.st_size%1024?1:0) ;
    Channel::SELF_CONN_OK = true;

    int sock1 = swift::Listen(7001);
    ASSERT_TRUE(sock1>=0);

    int file = swift::Open("doc/sofi.jpg");
    int file2 = swift::Open("doc/apusapus.png");
    int file3 = swift::Open("doc/cc-states.png");
    FileTransfer* fileobj = FileTransfer::file(file);

    swift::SetTracker(Address("127.0.0.1",7001));

    Cache a("cache",194798);
    int copy = a.Open(swift::RootMerkleHash(file));
    int copy2 = a.Open(swift::RootMerkleHash(file2));
    int copy3 = a.Open(swift::RootMerkleHash(file3));
    int copy4 = a.Open(swift::RootMerkleHash(file));

    swift::Loop(TINT_SEC);

    a.Close(copy);
    a.Close(copy2);
    a.Close(copy3);
    a.Close(copy4);
//    int copy = swift::Open("doc/sofi-copy.jpg",fileobj->root_hash());

//    int count = 0;
//    while (swift::SeqComplete(copy)!=size && count++<600)
//        swift::Loop(TINT_SEC);
    ASSERT_EQ(size,swift::SeqComplete(copy));



    swift::Close(file);

    swift::Shutdown(sock1);
}

int main (int argc, char** argv) {
    swift::LibraryInit();
    testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
