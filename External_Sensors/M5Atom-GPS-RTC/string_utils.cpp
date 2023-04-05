#include "string_utils.h"
//-----------------------------------------------------------------------------
void Str2Array(int len, int fieldcount, char *dlm, char *ary, char *fields) {
  int k, i;
  int startpos = 0;

  for (k = 0; k < fieldcount - 1; k++) {
    i = strpos(dlm, fields + startpos);
    memset(ary + (k * len), 0, len);
    strncpy(ary + (k * len), fields + startpos, i);
    startpos += (i + 1);
  }
  strcpy(ary + (k * len), fields + startpos);
}
//-----------------------------------------------------------------------------
int strpos(char *needle, char *haystack) {
  char *offset;
  offset = strstr(haystack, needle);
  if (offset != NULL) {
    return offset - haystack;
  }
  return -1;
}