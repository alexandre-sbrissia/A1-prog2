#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gbv.h" 

/*cria uma Library nova*/
int lib_create(Library *lib, const char *filename) {

  FILE *f ;
  struct stat data ;
  char sig[5] ;

  if (!lib) {
    perror("erro lib_create\n") ;
    exit(1) ;
  }
  
  f = fopen(filename, "r+") ;

  if (!f) {
    perror("erro na abertura do arquivo\n") ;
    exit(1) ;
  }

  stat(filename, &data) ;

  //arquivo novo
  if (data.st_size == 0) {
    
    fwrite("gbva", 1, 4, f) ;
    lib->docs = NULL ;
    lib->count = 0 ;
    fwrite(lib, 1, sizeof(Library), f) ;  
  }

  //testa se é um gbv
  fread (sig, 1, 4, f) ;

  if (strcmp(sig, "gbva") != 0) {
    perror("arquivo não é uma biblioteca\n") ;
    exit(1) ;
  }

  printf("passou\n") ;
  fclose(f) ;

  return 0 ;
}

/*cria a biblioteca*/
int gbv_create(const char *filename) {

  FILE *f ;
  Library *lib ;

  lib_create(lib, filename) ;

  lib = malloc(sizeof(Library)) ;

  return 0 ;
}

/*cria a struct Library para as demais operacoes*/
int gbv_open(Library *lib, const char *filename) {

  FILE *f ;
  char sig[5] ;
  struct stat data ;

  if (!lib) {
    perror("erro lib_create\n") ;
    exit(1) ;
  }

  f = fopen(filename, "a+") ;
  if (!f) {
    perror("erro na abertura do arquivo\n") ;
    exit(1) ;
  }

  /*biblioteca nova*/
  stat(filename, &data) ;
  if (data.st_size == 0) {
    lib_create(lib, filename) ;
    fclose(f) ;
    return 0 ;
  }

  fread (sig, 1, 4, f) ;
  if (strcmp(sig, "gbva") != 0) {
    perror("arquivo não é uma biblioteca\n") ;
    exit(1) ;
  }

  fclose(f) ;
  return 0 ;
}

int gbv_add(Library *lib, const char *archive, const char *docname)
{return 0;} 


int gbv_remove(Library *lib, const char *docname)
{return 0;} 


int gbv_list(const Library *lib)
{return 0;} 


int gbv_view(const Library *lib, const char *docname)
{return 0;} 


int gbv_order(Library *lib, const char *archive, const char *criteria)
{return 0;
} 


