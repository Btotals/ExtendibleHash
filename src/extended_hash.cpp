#include "extended_hash.h"

#define INPUT_BUFFER (page[PAGE_NUMBER-2].ptr)

int Hash::read_tuple_from_file(int offset) {
  FILE* CurrentPointer = fopen("?", "rb+");
  int PageLength = PAGE_SIZE, OffsetLength = 0;

  if (fseek(CurrentPointer, offset+PAGE_SIZE, SEEK_SET) == -1) {
    fseek(CurrentPointer, 0L, SEEK_END);
    PageLength = ftell(CurrentPointer) - offset;
    fread(INPUT_BUFFER, PageLength-1, 1, CurrentPointer);
  }
  else {
    fread(INPUT_BUFFER, PAGE_SIZE-1, 1, CurrentPointer);
  }
  INPUT_BUFFER[PageLength-1] = '\0';
  
  for (int i = 0; i < PageLength; i++) {
    if (INPUT_BUFFER[i] == '\n') {
      Tuple TempTuple;
      TempTuple.length = i - OffsetLength;
      memcpy(TempTuple.data, INPUT_BUFFER+OffsetLength, i - OffsetLength);

      char key[20];
      for (int j = 0; INPUT_BUFFER[j] != '|'; j++)
        key[j] = INPUT_BUFFER[j];
      TempTuple.key = atoi(key);

      OffsetLength = 0;
    }
    else {
      OffsetLength++;
    }
  }
  PageLength -= OffsetLength;
  return PageLength;
}

int main() {
  
}

