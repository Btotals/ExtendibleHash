#ifndef EH_H
#define EH_H
#include <iostream>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <list>
#include <stdio.h>
using namespace std;
#define PAGE_NUMBER 128								//页面个数
#define PAGE_SIZE 8192								//页面大小
class Page {
  public:
  	int pageid;  
  	int used_size;  //the size had been used
  	int Bucketid;  
  	bool locked;
  	bool accessed;  //have been written or read
  	bool dirty;		//have been written
  	bool is_used;
  	char* ptr;		//the address of the page start
  	Page() {
  	  ptr = new char[PAGE_SIZE];
      // memset(ptr, 0, PAGE_SIZE);
  	  used_size = 0;
  	  accessed = false;
  	  locked = false;
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
  	int on_pageid;
  	int position;	// the position in the file
	int *tail;
  	Bucket() {
  	  position = -1;
  	  on_pageid = -1;
  	}
};
class Tuple {
  public:
  	char data[200];
  	int length;
	int key;		//the  L_PARTKEY
	Tuple(char* c) {
	  int i;
	  for (i = 0; c[i] != '\n'; i++) data[i] = c[i];
	  data[i] = '\n';
	  length = i+1;
	  int num = 0;
	  for (i = 0; c[i] != '|'; i++) {
	    num = num*10+(c[i]-'0');
	  }
	  key = num;
	}
};
class entry{
public:
	int depth;
	int key;
	entry(int k = 0, int d = 0) {
		depth = d;
		key = k;
	}
	friend bool operator<(entry a, entry b) {
		return a.key < b.key;
	}

};
class Hash{
  public:
  	Hash();
  	~Hash();
	//****** the function of hash operation *********
  	int get_page();
	int evict_page();
	void read_tuple();
	void insert(Tuple t);
	int split(int pageid);
	void inc_localdepth(int bucketid, int pos);
	void double_the_index();
	void incfunction(int bid, int local, int pos);
	void update_the_key(int bid, int local, entry* &t);
	unsigned int global_key(Tuple t); // the key according to the global depth
	unsigned int global_key(int t);
	unsigned int final_key(Tuple t); // the truly key according to the local depth
	unsigned int final_key(int t);
	
	void search();
	
	//****** the function of read and write *********
	void write_bucket_to_file(int pageid); //write back the bucket and clean the page
	void write_index_to_file();
	void read_index_from_file(int offset);
	bool read_tuple_from_file(int offset);
	void read_bucket_from_file(int backetid, int pageid, int key);
	
	//****** the member of hash *********
	Page page[PAGE_NUMBER];  //page[PAGE_NUMBER-2] used to be the input buffer, page[PAGE_NUMBER-1] used to be the catalogue
	Bucket bucket[PAGE_NUMBER-2];
	int global_depth;
	int index_offset;
	int mode;
	int IOs;
	int total_bk;  // the number of bucket
	int* Index;
};






#endif
