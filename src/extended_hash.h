#ifndef EH_H
#define EH_H
#include <iostream>
#include <cstring>
#include <cstdlib>
using namespace std;
#define PAGE_NUMBER 8               //the number of pages
#define PAGE_SIZE 8192                //the size of pages
class Page {
  public:
    int pageid;  
    int used_size;  //the size had been used
    int Bucketid;  
    bool accessed;  //have been written or read
    bool dirty;   //have been written
    bool is_used;
    char* ptr;    //the address of the page start
    Page() {
      ptr = new char[PAGE_SIZE];
      memset(ptr, 0, PAGE_SIZE);
      used_size = 0;
      accessed = false;
      dirty = false;
      is_used = false;
      Bucketid = -1;
    }
    ~Page() {
      delete []ptr;
    }
};
class Bucket {
  public:
    int depth;
    int Bucketid; 
    int on_pageid;
    int position; // the position in the file
    Bucket() {
      Bucketid = -1;
      on_pageid = -1;
    }
};
class Tuple {
  public:
    char data[200];
    int length;
    int key;    //the  L_PARTKEY
};
class Hash{
  public:
    Hash();
    ~Hash();
    int get_page();
    int evict_page();
    void insert(Tuple t);
    int split(int pageid);
    int global_key(Tuple t); // the key according to the global depth
    int final_key(Tuple t); // the truly key according to the local depth
    //int findkey(int fkey);
    
    void write_bucket_to_file(int pageid); //write back the bucket and clean the page
    void write_index_to_file();
    void read_index_from_file(int offset);
    bool read_tuple_from_file(int offset);
    void read_bucket_from_file(int backetid, int pageid);
    
    
    Page page[PAGE_NUMBER];  //page[PAGE_NUMBER-2] used to be the input buffer, page[PAGE_NUMBER-1] used to be the catalogue
    Bucket bucket[PAGE_NUMBER-2];
    int global_depth;
    int index_offset;
    int mode;
};






#endif
