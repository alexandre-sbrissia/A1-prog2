#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gbv.h"
#include "lib.h"

/*desaloca biblioteca*/
int lib_destroy(Library *lib) {

  if (!lib)
    return 0 ;

  free(lib->docs) ;

  return 0 ;
}

/*move o arc criando um espaço de data_size bytes na posicao pos*/
int lib_move(const char* arc_name, long pos, long data_size) {

  unsigned char buffer[BUFFER_SIZE] ;
  long bytes_left, offset_orig, offset_dest ;
  long bloco_size, read, arc_size, i ;
  int fd ;

  FILE* arc = fopen(arc_name, "rb+");
  if (arc == NULL) {
    printf("Erro ao abrir o arquivo.\n") ;
    return -1 ;
  }

  /*calcula tamanho do arquivo*/
  fseek(arc, 0, SEEK_END) ;
  arc_size = ftell(arc) ;
  
  if (pos < 0 || pos > arc_size) {
    printf("Posição inválida.\n") ;
    fclose(arc);
    return -1 ;
  }

  if (data_size == 0) {
    fclose(arc);
    return 0 ;
  }

  /*preenche o arquivo para expandir
    e mover esse conteudo para o meio*/
  fseek(arc, 0, SEEK_END);
  for (i = 0; i < data_size; i++) {
    fputc(0, arc) ;
  }

  /*desloca dados do final para posiçao */
  bytes_left = arc_size - pos ;
  
  while (bytes_left > 0) {

    /*calcula o tamanho do próximo bloco a mover*/
    bloco_size = (bytes_left > BUFFER_SIZE) ? BUFFER_SIZE : bytes_left ;
    
    /*calcula offsets*/
    offset_orig = pos + bytes_left - bloco_size ;
    offset_dest = offset_orig + data_size ;
    
    /*leitura do bloco*/
    fseek(arc, offset_orig, SEEK_SET) ;
    memset(buffer, 0, BUFFER_SIZE) ;
    read = fread(buffer, 1, bloco_size, arc) ;
    
    if (read == 0) break ;
    
    /*escreve bloco*/
    fseek(arc, offset_dest, SEEK_SET) ;
    fwrite(buffer, 1, read, arc) ; 
    
    bytes_left -= read ;
  }

  /*corta o tamanho do arquivo*/
  /*nao foi possivel utilizar truncate*/
  if (data_size < 0) {
    fflush(arc) ;
    fd = fileno(arc) ;
    if(ftruncate(fd, arc_size + data_size) == -1)
      perror("erro truncate\n") ;
  }

  fclose(arc) ;
  return 0 ;
}

/*retorna um vetor ordenado com o criterio.*/  
/*o vetor precisa ser desalocado depois do uso*/
Document *doc_sort(Library *lib, const char *criteria) {

  int i, j ;
  Document tmp, *aux ;

  if (!lib || !criteria) {

    perror("erro doc_sort\n") ;
    return NULL ;
  }
  if (strcmp(criteria,"nome") != 0 && strcmp(criteria,"data") != 0
      && strcmp(criteria,"tamanho") != 0) {
    perror("criterio invalido \n") ;
    return NULL ;
  }
  
  /*copiando o vetor*/
  aux = malloc(sizeof(Document) * lib->count) ;  
  if (!aux) {
    perror("erro de alocacao doc_sort\n") ;
    exit(1) ;
  }

  for (i = 0; i < lib->count; i++)
    aux[i] = lib->docs[i] ; 

  if (strcmp(criteria,"nome") == 0)  {

    //selection sort
    for (i = 0; i < lib->count -1; i++) 
      for (j = 0; j < lib->count; j++) 
        
        if (strcmp(aux[i].name, aux[j].name) > 0){
          tmp = aux[i] ;
          aux[i] = aux[j] ;
          aux[j] = tmp ;
        }
  }
  else if (strcmp(criteria,"data") == 0) {

    for (i = 0; i < lib->count -1; i++) 
      for (j = 0; j < lib->count; j++) 
        
        if (aux[i].date > aux[j].date){
          tmp = aux[i] ;
          aux[i] = aux[j] ;
          aux[j] = tmp ;
        }
  }
  else if (strcmp(criteria,"tamanho") == 0) {

    for (i = 0; i < lib->count -1; i++) 
      for (j = 0; j < lib->count; j++) 
        if (aux[i].size > aux[j].size){
          tmp = aux[i] ;
          aux[i] = aux[j] ;
          aux[j] = tmp ;
        }
  }

  return aux ;
}
