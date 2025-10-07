#ifndef LIB_H
#define LIB_H

#include "gbv.h"

int lib_destroy(Library *lib) ;
int lib_move(const char* arc_name, long pos, long data_size) ;
Document *doc_sort(Library *lib, const char *criteria) ;

#endif
