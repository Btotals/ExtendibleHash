#include"extended_hash.h"

#define  INPUT (page[PAGE_NUMBER-2])
#define  INDEX (page[PAGE_NUMBER-1])
#define  INT sizeof(int)
char Index[100];
char BUCKET[100];
char IN[100];
char OUT[100];
char TBL[100];
char TEMP[100];
int FileSize = 0;

Hash h;
int main(int argc, char ** argv) {
	/*
  char c[20];
  for (int i = 0; i < 20; i++) c[i] = 'a';
  c[0] = '1';
  c[1] = '|';
  c[19] = '\n';
  for (int i = 0; i < 300; i++)
  h.insert(Tuple(c));
  c[0] = '0';
  for (int i = 0; i < 300; i++)
  h.insert(Tuple(c));



  for (int i = 0; i < PAGE_NUMBER-2; i++) {
    if (h.page[i].is_used) {
      cout << h.page[i].Bucketid << endl;
      cout << h.page[i].used_size << endl << -1 << endl;
    }
  }
  */
  strcpy(BUCKET, argv[1]);
  strcat(BUCKET, "\\hashbucket.out");
  strcpy(Index, argv[1]);
  strcat(Index, "\\hashindex.out");
  strcpy(IN, argv[1]);
  strcat(IN, "\\testinput.in");
  strcpy(OUT, argv[1]);
  strcat(OUT, "\\testoutput.out");
  strcpy(TBL, argv[1]);
  strcat(TBL, "\\lineitem.tbl");

  FILE* CurrentPointer = fopen(TBL, "rb+");
  fseek(CurrentPointer, 0, SEEK_END);
  FileSize = ftell(CurrentPointer);
  fclose(CurrentPointer);

  FILE* fp = fopen(BUCKET, "wb+");
  fclose(fp);
  fp = fopen(Index, "wb+");
  fclose(fp);
  fp = fopen(OUT, "wb+");
  fclose(fp);
  cout << "开始创建索引\n";
  h.read_tuple();
  h.search();
  cout << "Complete\n IO is : " << h.IOs << endl;
  cout << "the number of bucket is : " << h.total_bk << endl;
}

Hash::Hash() {
  global_depth = 1;
  IOs = 0;
  total_bk = 0;
  Index = (int*)page[PAGE_NUMBER - 1].ptr;
  for (int i = 0; i < PAGE_NUMBER - 2; i++) {
	  bucket[i].tail = (int*)page[i].ptr + (PAGE_SIZE / INT) - 1;
  }
  mode = 0; //least mode
  page[0].is_used = true;
  page[0].Bucketid = 0;
  page[1].is_used = true;
  page[1].Bucketid = 1;
  bucket[0].depth = 1;
  bucket[1].depth = 1;
  *bucket[0].tail = 1;
  *bucket[1].tail = 1;
  bucket[0].position = 0;
  bucket[1].position = 1;
  Index[0] = total_bk++;
  Index[1] = total_bk++;
  page[PAGE_NUMBER-1].used_size = 2*INT;
}

Hash::~Hash() {
}

int Hash::get_page() {  //get a empty page, or evict a page from the memory, return the pageid
  for (int i = 0; i < PAGE_NUMBER-2; i++) {
    if (!page[i].is_used) {
      page[i].is_used = true;
      return i;
    }
  }
  return evict_page();
}

int Hash::evict_page() { //choose a page to evict and clean it, return the pageid
  for (int i = 0;; i++) {
  	if (i == PAGE_NUMBER-2) i = 0;
  	if (!page[i].locked){

    if (page[i].accessed) {
      page[i].accessed = false;
    } else {
      write_bucket_to_file(i);
      // memset(page[i].ptr,0, PAGE_SIZE);
      for (int j = 0; j < PAGE_SIZE; j++)
        page[i].ptr[j] = 0;
      page[i].used_size = 0;
      return i;
    }

	}
  }
}

unsigned int Hash::global_key(Tuple t) {
  unsigned int temp = t.key;
  int jump = 32-global_depth;
  if (mode == 1) {
  	temp = (temp << jump) >> jump;
  }
  else {
	  int a[23];
	  for (int i = 0; i < 23; i++) {
		  a[i] = temp % 2;
		  temp /= 2;
	  }
	  for (int i = 23-global_depth; i < 23; i++) temp = temp * 2 + a[i];
  }
  return temp;
}

unsigned int Hash::global_key(int t) {
	unsigned int temp = t;
	int jump = 32 - global_depth;
	if (mode == 1) {
		temp = (temp << jump) >> jump;
	}
	else {
		int a[23];
		for (int i = 0; i < 23; i++) {
			a[i] = temp % 2;
			temp /= 2;
		}
		for (int i = 23 - global_depth; i < 23; i++) temp = temp * 2 + a[i];
	}
	return temp;
}

void Hash::inc_localdepth(int bucketid, int pos) {
	if (bucketid / (PAGE_SIZE / INT) != index_offset) {
		write_index_to_file();
		read_index_from_file(bucketid / (PAGE_SIZE / INT));
	}
	Index[bucketid % (PAGE_SIZE / INT)] = pos;
}

unsigned int Hash::final_key(Tuple t) {
	unsigned int gkey = global_key(t);
	if (gkey / (PAGE_SIZE / INT) != index_offset) {
		write_index_to_file();
		read_index_from_file(gkey / (PAGE_SIZE / INT));
	}
	int temp = Index[gkey % (PAGE_SIZE / INT)];
	return temp;
}

unsigned int Hash::final_key(int t) {
	unsigned int gkey = global_key(t);
	if (gkey / (PAGE_SIZE/INT) != index_offset) {
		write_index_to_file();
		read_index_from_file(gkey / (PAGE_SIZE / INT));
	}
	int temp = Index[gkey % (PAGE_SIZE / INT)];
	return temp;
}

void Hash::insert(Tuple t){
	int gkey = global_key(t);
  int hkey = final_key(t);
  int index = -1, index2;
  //search the bucket of key(hkey)
  for (int i = 0; i < PAGE_NUMBER-2; i++) {
    if (hkey == bucket[i].position) {
      index = i;
      break;
    }
  }
  //if the bucket was not in the memory, get a page
  if (index == -1) {
    index = get_page();
    read_bucket_from_file(hkey, index, gkey);
  }
  //if the page is overflow, split
  while (page[index].used_size+t.length > PAGE_SIZE-INT) {
  	page[index].locked = true;
    index2 = split(index);
    page[index].locked = false;
    hkey = final_key(t);
	if (bucket[index2].position == hkey) {
		index = index2;
	}
  }
  //insert the tuple
  int temp = page[index].used_size;
  for (int i = 0; i < t.length; i++) {
    page[index].ptr[temp++] = t.data[i];
  }
  page[index].used_size = temp;
  page[index].accessed = true;
}

int Hash::split(int pageid){
  int bid = page[pageid].Bucketid;
  unsigned int bid2 = 1;
  bid2 = (bid2<<bucket[pageid].depth)+bid;
  int newid = get_page();
  page[newid].Bucketid = bid2;
  bucket[newid].position = total_bk++;
  /*
  if (bid2 / PAGE_SIZE != index_offset) {
	  //write_index_to_file();
	  read_index_from_file(bid2 / PAGE_SIZE);
  }
  page[PAGE_NUMBER - 1].ptr[bid2%PAGE_SIZE] = bucket[pageid].depth;
  */
  if (global_depth == bucket[pageid].depth) {
    double_the_index();
  }
  incfunction(bid, bucket[pageid].depth, total_bk-1);
  int len = page[pageid].used_size;
  bucket[pageid].depth = bucket[newid].depth = bucket[pageid].depth+1;
  *bucket[pageid].tail = bucket[pageid].depth; *bucket[newid].tail = bucket[pageid].depth;
  page[pageid].used_size = page[newid].used_size = 0;
  char *c = page[pageid].ptr;
  char *d = page[newid].ptr;
  int dep = bucket[pageid].depth;
  for (int i = 0; i < len;) {
  	unsigned int num = 0;
  	for (int j = i; c[j] != '|'; j++) {
  	  num = num*10+(c[j]-'0');
  	}
  	if (mode == 1) {
  	  num = (num<<(32-dep))>>(32-dep);
	}
	else {
		int a[23];
		for (int k = 0; k < 23; k++) {
			a[k] = num % 2;
			num /= 2;
		}
		for (int k = 23 - dep; k < 23; k++) num = num * 2 + a[k];
	}
  	if (num == page[pageid].Bucketid) {
  	  for (int j = i; c[j] != '\n'; j++) {
  	  	c[page[pageid].used_size++] = c[j];
  	  }
  	  c[page[pageid].used_size++] = '\n';
  	} else {
  	  for (int j = i; c[j] != '\n'; j++) {
  	  	d[page[newid].used_size++] = c[j];
  	  }
  	  d[page[newid].used_size++] = '\n';
  	}
  	i = page[pageid].used_size+page[newid].used_size;
  }
  for (int i = page[pageid].used_size; i < PAGE_SIZE-INT; i++) c[i] = 0;
  return newid;
}
void Hash::double_the_index() {
  int totalindex = 1;
  for (int i = 0; i < global_depth; i++) totalindex *= 2;  //calculate the total number of index

  if (totalindex/(PAGE_SIZE/INT) == 0) {  // which means that a page is big enough to hold all the index
  	for (int i = 0; i < totalindex; i++) {
  	  Index[i+totalindex] = Index[i];
  	}
  	page[PAGE_NUMBER-1].used_size *= 2;
  	write_index_to_file();
  } else {
  	  int count = totalindex/(PAGE_SIZE/INT);
  	  write_index_to_file();
  	  for (int i = 0; i < count; i++) {
  	    read_index_from_file(i);
  	    index_offset += count;
  	    write_index_to_file();
  	  }
  }
  global_depth++;
}

void Hash::search(){
	write_bucket_to_file(0);
	page[0].used_size = 0;
	page[0].locked = true;
	// memset(page[0].ptr, 0, PAGE_SIZE);
  for (int j = 0; j < PAGE_SIZE; j++)
    page[0].ptr[j] = 0;
	FILE* CurrentPointer = fopen(IN, "rb+");
	fseek(CurrentPointer, 0, SEEK_END);
	int size = ftell(CurrentPointer);
	fseek(CurrentPointer, 0, SEEK_SET);
	fread(INPUT.ptr, size, 1, CurrentPointer);
	fclose(CurrentPointer);
	INPUT.used_size = size;
	int n = 0;
	int i;
	for (i = 0; INPUT.ptr[i] != '\r'; i++) {
		n = n * 10 + (INPUT.ptr[i] - '0');
	}
	while (n--) {
		int key = 0;
		for (i += 2; INPUT.ptr[i] != '\r'; i++) key = key * 10 + (INPUT.ptr[i] - '0');

		unsigned int k = final_key(key);
		int index = -1;
		for (int x = 1; x < PAGE_NUMBER - 2; x++) {
			if (bucket[x].position == k) {
				index = x;
				break;
			}
		}
		if (index == -1) {
			index = get_page();
			int gkey = global_key(key);
			read_bucket_from_file(k, index, gkey);
		}
		char *c = page[index].ptr;
		int length = page[index].used_size;
		for (int j = 0; j < length;) {
			Tuple t(c);
			if (t.key == key) {
				for (int z = 0; z < t.length; z++) page[0].ptr[page[0].used_size++] = t.data[z];
			}
			j += t.length;
			c += t.length;
		}
	}
	page[0].ptr[page[0].used_size++] = 0;
	cout << page[0].ptr << endl;

	CurrentPointer = fopen(OUT, "rb+");
	fseek(CurrentPointer, 0, SEEK_SET);
	fwrite(page[0].ptr, page[0].used_size, 1, CurrentPointer);
	fclose(CurrentPointer);
}

void Hash::incfunction(int bid, int local, int pos) {
	int count = pow(2, global_depth - local - 1);
	entry *e = new entry[count];
	unsigned int r = 1 << local;
	entry *temp = e;
	update_the_key(bid + r, local + 1, temp);
	sort(e, e+count);
	for (int i = 0; i < count; i++) inc_localdepth(e[i].key, pos);
	delete[]e;
}

void Hash::update_the_key(int bid, int local, entry* &t) {
	if (local == global_depth) {
		*t = entry(bid, local);
		t++;
		return;
	}
	unsigned int r = 1 << local;
	update_the_key(bid, local + 1, t);
	update_the_key(bid+r, local + 1, t);
}

void Hash::read_tuple() {
	int temp = 0;
	while (read_tuple_from_file(temp)) {
		temp += page[PAGE_NUMBER - 2].used_size;
		char *c = page[PAGE_NUMBER - 2].ptr;
		int length = page[PAGE_NUMBER - 2].used_size;
		for (int i = 0; i < length;) {
			Tuple t(c);
			insert(t);
			i += t.length;
			c += t.length;
		}
	}
}

void Hash::write_bucket_to_file(int pageid){
	FILE* CurrentPointer = fopen(BUCKET, "rb+");
	int bucketid = bucket[pageid].position;

	fseek(CurrentPointer, bucketid*PAGE_SIZE, SEEK_SET);
	fwrite(page[pageid].ptr, PAGE_SIZE, 1, CurrentPointer);
	IOs++;
	fclose(CurrentPointer);
}

void Hash::write_index_to_file(){
	FILE* CurrentPointer = fopen(Index, "rb+");

	fseek(CurrentPointer, index_offset*PAGE_SIZE, SEEK_SET);
	fwrite(INDEX.ptr, PAGE_SIZE, 1, CurrentPointer);
	IOs++;
	fclose(CurrentPointer);
}

void Hash::read_index_from_file(int offset){
	FILE* CurrentPointer = fopen(Index, "rb+");

	fseek(CurrentPointer, offset*PAGE_SIZE, SEEK_SET);
	fread(INDEX.ptr, PAGE_SIZE, 1, CurrentPointer);
	index_offset = offset;
	IOs++;
	fclose(CurrentPointer);
}
bool Hash::read_tuple_from_file(int offset){
	FILE* CurrentPointer = fopen(TBL, "rb+");
	int ReadSize = offset + PAGE_SIZE > FileSize ? FileSize - offset : PAGE_SIZE;

	fseek(CurrentPointer, offset, SEEK_SET);
	fread(INPUT.ptr, ReadSize, 1, CurrentPointer);

	while (INPUT.ptr[ReadSize - 1] != '\n')
		ReadSize--;

	INPUT.used_size = ReadSize;
	fclose(CurrentPointer);
	IOs++;
	return offset + PAGE_SIZE > FileSize ? false : true;

}
void Hash::read_bucket_from_file(int bucketid, int pageid, int key){
	FILE* CurrentPointer = fopen(BUCKET, "rb+");
	/*
	if (bucketid / (PAGE_SIZE / INT) != index_offset) {
		write_index_to_file();
		read_index_from_file(bucketid / (PAGE_SIZE / INT));
	}
	int pos = Index[bucketid % (PAGE_SIZE / INT)];
	*/
	fseek(CurrentPointer, bucketid*PAGE_SIZE, SEEK_SET);
	fread(page[pageid].ptr, PAGE_SIZE, 1, CurrentPointer);
	//page[pageid].Bucketid = bucketid;
	page[pageid].used_size = PAGE_SIZE-INT;

	bucket[pageid].position = bucketid;

	bucket[pageid].depth = *bucket[pageid].tail;
	int jump = 32 - bucket[pageid].depth;
	unsigned int r = key;
	r = (r << jump) >> jump;
	page[pageid].Bucketid = r;
	for (int i = PAGE_SIZE - INT -1; i >= 0; i--) {
		if (page[pageid].ptr[i] == '\n') break;
		page[pageid].used_size--;
	}
	IOs++;
	fclose(CurrentPointer);
}