#include "string_utils.h"
//-----------------------------------------------------------------------------
int Str2Array(int token_count, int token_length, char* dlm, char *source_string, char *dest_array) {
  char *token;
  int k=0;
  int len;
  token = strtok(source_string, dlm);
  while(token !=NULL) {
    if(k < token_count) {
      len = (int) strlen(token);
      if(len < token_length)  strcpy(dest_array, token);
      else {
        strncpy(dest_array,token,(size_t) token_length);
        dest_array[token_length-1] = (char) 0x00;
      }
      dest_array = dest_array + token_length;
      k++;
    }
    token = strtok(NULL,dlm);
  }
  return k;
 }
