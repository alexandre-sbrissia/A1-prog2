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
  
  f = fopen(filename, "rb+") ;

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
    fwrite(&lib->count, 1, sizeof(int), f) ;  
  }

  //testa se é um gbv
  fseek(f, 0, SEEK_SET) ;
  fread (sig, 1, 4, f) ;
  printf("aaa %s\n", sig) ;

  if (strcmp(sig, "gbva") != 0) {
    perror("arquivo não é uma biblioteca lib create\n") ;
    exit(1) ;
  }

  printf("passou\n") ;
  fclose(f) ;

  return 0 ;
}

/*cria a biblioteca*/
int gbv_create(const char *filename) {

  Library *lib ;

  lib = malloc(sizeof(Library)) ;

  lib_create(lib, filename) ;

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

  f = fopen(filename, "ab+") ;
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

  fread(sig, 1, 4, f) ;
  if (strcmp(sig, "gbva") != 0) {
    perror("arquivo não é uma biblioteca\n") ;
    exit(1) ;
  }

  /*montagem da lib*/
  fread(&lib->count, 1, 4, f) ; 

  for(int i = 0; i < lib->count; i++){

    fread(&lib->docs[i], 1, sizeof(Document), f) ;
  }

  fclose(f) ;
  return 0 ;
}

int gbv_add(Library *lib, const char *archive, const char *docname) {

  FILE *farq, *fdoc ;
  Document doc ;
  struct stat data ;
  char buf[BUFFER_SIZE] ;
  long p ;

  if (!lib || !archive || !docname) {
    perror("erro gbv_add\n") ;
    exit(1) ;
  }
  farc = fopen(docname, "ab+") ;
  if (!f) {
    perror("erro na abertura do arquivo\n") ;
    exit(1) ;
  }
  fdoc = fopen(docname, "rb+") ;
  if (!f) {
    perror("erro na abertura do arquivo\n") ;
    exit(1) ;
  }

  /*criando a estrutura do documento*/
  stat(docname, &data) ;
  doc.name = docname ;
  doc.size =(long) data.st_size ;
  doc.date = (time_t) 0 ;  /*"data da insersao"*/
  doc.offset = ; /*nao sei ainda*/

  /*testa se o vetor ja contem o doc*/

  /*calcula onde a estrutura vai ficar*/
  p = (lib->count) * sizeof(Document) ;
  p += 8 ;
  fseek(farc, p, SEEK_END) ;

  /*move o archive para dar espaço*/
  for(int i = 0; i < p )
    fread(buf, 

  /*escreve no diretorio a estrutura*/
  /*escreve no archive o conteúdo*/

  return 0;
} 



int gbv_remove(Library *lib, const char *docname){
  return 0;
} 


int gbv_list(const Library *lib) {
  return 0;
} 


int gbv_view(const Library *lib, const char *docname) {
  return 0;
} 


int gbv_order(Library *lib, const char *archive, const char *criteria) {
  return 0;
} 


