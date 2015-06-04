#include "extended_hash.h"

#define INPUT_PAGE (page[PAGE_NUMBER-2])
#define INDEX_PAGE (page[PAGE_NUMBER-1])

bool Hash::read_tuple_from_file(int offset) {
  FILE* CurrentPointer = fopen("?", "rb+");
  int PageLength = PAGE_SIZE, OffsetLength = 0;

  if (fseek(CurrentPointer, offset+PAGE_SIZE, SEEK_SET) == -1) {
    fseek(CurrentPointer, 0L, SEEK_END);
    PageLength = ftell(CurrentPointer) - offset;
    fread(INPUT_PAGE.ptr, PageLength, 1, CurrentPointer);
    INPUT_PAGE.used_size = PageLength;

    return false;
  }

  fread(INPUT_PAGE.ptr, PAGE_SIZE, 1, CurrentPointer);
  
  if (INPUT_PAGE.ptr[PAGE_SIZE-1] != '\n') {
    while (INPUT_PAGE.ptr[OffsetLength] != '\n')
      OffsetLength--;
  }

  INPUT_PAGE.used_size = OffsetLength;

  fclose(CurrentPointer);

  return true;
}

void Hash::read_index_from_file(int offset) {
  FILE* CurrentPointer = fopen("?", "rb+");

  fseek(CurrentPointer, offset*PAGE_SIZE, SEEK_SET);
  fread(INDEX_PAGE.ptr, PAGE_SIZE, 1, CurrentPointer);
  INDEX_PAGE.used_size = PAGE_SIZE;

  fclose(CurrentPointer);
}

void Hash::read_bucket_from_file(int bucketid, int pageid) {
  FILE* CurrentPointer = fopen("?", "rb+");

  fseek(CurrentPointer, bucketid*PAGE_SIZE, SEEK_SET);
  fread(page[pageid].ptr, PAGE_SIZE, 1, CurrentPointer);
  page[pageid].used_size = PAGE_SIZE;

  fclose(CurrentPointer);
}

void Hash::write_bucket_to_file(int pageid) {
  int bucketid = page[pageid].Bucketid;
  FILE* CurrentPointer = fopen("?", "rb+");

  fseek(CurrentPointer, bucketid*PAGE_SIZE, SEEK_SET);
  fwrite(page[pageid].ptr, page[pageid].used_size, 1, CurrentPointer);

  fclose(CurrentPointer);
}

void Hash::write_index_to_file() {
  int FirstIndex = INDEX_PAGE.ptr[0];
  int bucketid = FirstIndex/PAGE_SIZE;
  FILE* CurrentPointer = fopen("?", "rb+");

  fseek(CurrentPointer, bucketid*PAGE_SIZE, SEEK_SET);
  fwrite(INDEX_PAGE.ptr, INDEX_PAGE.used_size, 1, CurrentPointer);

  fclose(CurrentPointer);
}

int main() {

}

