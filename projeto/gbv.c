#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gbv.h"
#include "lib.h"

/*cria a biblioteca*/
int gbv_create(const char *filename) {

  FILE *f ;
  struct stat data ;
  Library lib ;

  f = fopen(filename, "rb+") ;
  if (!f) {
    perror("erro na abertura do arquivo\n") ;
    exit(1) ;
  }

  stat(filename, &data) ;

  //arquivo novo
  if (data.st_size == 0) {
    
    fwrite("gbva", 1, 4, f) ;
    lib.count = 0 ;
    lib.docs = NULL ;
    fwrite(&lib.count, 1, sizeof(int), f) ;  
  }

  fclose(f) ;
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
    gbv_create(filename) ;
    lib->count = 0 ;
    lib->docs = NULL ;
  }

  freopen(filename, "rb", f) ;

  /*testa se é uma Lib*/
  fread(sig, 1, 4, f) ;
  if (strncmp(sig, "gbva", 4) != 0) {
    perror("arquivo não é uma biblioteca\n") ;
    exit(1) ;
  }

  /*montagem da lib*/
  fread(&lib->count, 1, 4, f) ; 

  /*aloca o vetor de documentos*/
  if (lib->count > 0) {
    lib->docs = malloc(lib->count * sizeof(Document));
    if (!lib->docs) {

      perror("erro malloc docs");
      fclose(f);
      exit(1);
    }
    for(int i = 0; i < lib->count; i++){
      fread(&lib->docs[i], 1, sizeof(Document), f) ;
    }
  } else {
    lib->docs = NULL;
  }

  fclose(f) ;
  return 0 ;
}

/*falta manipular caso de arquivo de mesmo nome*/
int gbv_add(Library *lib, const char *archive, const char *docname) {

  FILE *farc, *fdoc ;
  Document doc ;
  struct stat data ;
  char buf[BUFFER_SIZE] ;
  int new_count ; 
  long dir_pos, left, n_buf, n_read ;

  if (!lib || !archive || !docname) {
    perror("erro gbv_add\n") ;
    exit(1);
  }
  /* Verifica duplicidade*/
  for (int i = 0; i < lib->count; i++) 
    if (strcmp(lib->docs[i].name, docname) == 0) {
      
      gbv_remove(lib, archive, docname) ;
      break ;
    }

  fdoc = fopen(docname, "rb") ;
  if (!fdoc) {
    perror("erro na abertura do arquivo\n") ;
    return -1 ;
  }

  /* Preenche a estrutura do documento*/
  memset(&doc, 0, sizeof(Document)) ; 
  stat(docname, &data) ; 
  strncpy(doc.name, docname, MAX_NAME -1) ;
  doc.name[MAX_NAME - 1] = '\0' ;
  doc.date = time(NULL) ;
  fseek(fdoc, 0, SEEK_END) ;
  doc.size = ftell(fdoc) ;
  fseek(fdoc, 0, SEEK_SET) ;

  /*move e insere a estrutura Document no diretório*/
  dir_pos = 8 + lib->count * sizeof(Document) ; // 4 bytes sig + 4 bytes count
  lib_move(archive, dir_pos, sizeof(Document)) ;

  farc = fopen(archive, "rb+") ;
  if (!farc) {
    perror("erro na abertura do arquivo\n") ;
    fclose(fdoc) ;
    return -1 ;
  }

  fseek(farc, dir_pos, SEEK_SET) ;
  fwrite(&doc, 1, sizeof(Document), farc) ;

  /*Escreve o conteúdo do documento no final do arquivo da biblioteca */
  fseek(farc, 0, SEEK_END) ;
  doc.offset = ftell(farc) ;

  /*calcula quanto falta para escrever*/
  /*caso seja maior que o tamanho do buf pega o restante*/
  left = doc.size ;
  while (left > 0) {

    memset(buf, 0, BUFFER_SIZE) ;  
    n_buf = (left > BUFFER_SIZE) ? BUFFER_SIZE : left ;
    n_read = fread(buf, 1, n_buf, fdoc) ;
    if (n_read == 0) break ;

    fwrite(buf, 1, n_buf, farc) ;
    left -= n_buf ;
  }
  fclose(fdoc) ;

  /*atualiza os demais offsets*/
  for (int i = 0; i < lib->count; i++) {

    lib->docs[i].offset += sizeof(Document) ;
    fseek(farc, 8 + i * sizeof(Document), SEEK_SET) ;
    fwrite(&lib->docs[i], sizeof(Document), 1, farc) ;
  }

  /* Corrijir o offset do novo Document */
  fseek(farc, dir_pos, SEEK_SET) ;
  fwrite(&doc, sizeof(Document), 1, farc) ;

  /*Atualiza o contador de documentos no cabeçalho*/
  new_count = lib->count + 1 ;
  fseek(farc, 4, SEEK_SET) ; // 4 bytes após a assinatura
  fwrite(&new_count, sizeof(int), 1, farc) ;

  fclose(farc) ;

  /* Atualiza o diretório em memória*/
  lib->docs = realloc(lib->docs, new_count * sizeof(Document));
  if (!lib->docs) {
    perror("erro realloc\n") ;
    return -1 ;
  }
  lib->docs[lib->count] = doc ;
  lib->count = new_count ;

  return 0 ;
}

int gbv_remove(Library *lib, const char *archive, const char *docname){

  int i, j ;
  long data_size, dir_pos ;
  Document *tmp ;

  if (!lib || !docname) {
    perror("erro gbv_remove\n");
    exit(1);
  }
  FILE *arc = fopen(archive, "rb+");
  if (!arc) {
    perror("erro na abertura do arquivo\n");
    exit(1);
  }

  /*acha o documento*/
  for (i = 0; i < lib->count; i++) {
    if (strcmp(lib->docs[i].name, docname) == 0)
      break ;
  }
  if (i == lib->count ) {
    perror("documento nao encontrado\n") ;
    fclose(arc) ;
    return -1 ;
  } 

  /*remove o conteudo*/
  data_size = lib->docs[i].size ;
  lib_move(archive, lib->docs[i].offset + data_size, -data_size) ;

  /*corrije os offsets e os index dos posteriores*/
  for (j = i +1; j < lib->count; j++) {
    lib->docs[j].offset -= data_size ;
    lib->docs[j-1] = lib->docs[j] ;
  }

  lib->count-- ;
  if(lib->count > 0)
    tmp = realloc(lib->docs, lib->count * sizeof(Document)) ;
  else {
    
    free(lib->docs) ;
    lib->docs = NULL ;
    tmp = NULL ;
  }

  if (lib->count > 0 && !tmp) {
    perror("erro realloc") ;
    fclose(arc) ;
    return -1 ;
  }
  lib->docs = tmp ;

  /*remove o espaco no diretorio*/
  /*calcula a posicao final do metadado*/
  dir_pos = 8 + i * sizeof(Document) ;
  lib_move(archive, dir_pos, -sizeof(Document)) ;

  /*atualiza todos os offsets nos metadados*/
  /*escreve na lib ARRUMAR*/
  for (j = 0; j < lib->count; j++) {

    fseek(arc, 8 + j * sizeof(Document), SEEK_SET);
    fwrite(&lib->docs[j], sizeof(Document), 1, arc);
  }

  /*reescreve a contagem*/
  fseek(arc, 4, SEEK_SET) ;
  fwrite(&lib->count, sizeof(int), 1, arc) ;

  fclose(arc) ;
  return 0;
} 

int gbv_list(const Library *lib) {

  if (!lib) {
    perror("erro gbv_list\n");
    exit(1);
  }
 
  for (int i = 0; i < lib->count; i++) {

    printf("\nDocumento %d:\n", i + 1); 
    printf("  Nome: %s\n", lib->docs[i].name);
    printf("  Tamanho: %ld bytes\n", lib->docs[i].size);
    printf("  Data: %s", ctime(&lib->docs[i].date));
    printf("  Offset: %ld\n", lib->docs[i].offset);
  }
  printf("\n") ;

  return 0;
} 

int gbv_view(const Library *lib, const char *archive, const char *docname) {

  int i ;
  long j, pos, data_size, data_offset, to_read, read ;
  char command, buf[BUFFER_SIZE] ;

  if (!lib || !docname || !archive) {
    perror("erro gbv_view\n");
    exit(1);
  }
  FILE *doc = fopen(archive, "rb");
  if (!doc) {
    perror("erro na abertura do arquivo\n");
    exit(1);
  }

  /*acha o documento no vetor lib->docs*/
  for (i = 0; i < lib->count; i++) {
    if (strcmp(lib->docs[i].name, docname) == 0)
      break ;
  }
  if (i == lib->count ) {
    perror("documento nao encontrado\n") ;
    fclose(doc) ;
    return -1 ;
  } 

  data_size = lib->docs[i].size ;
  data_offset = lib->docs[i].offset ;
  pos = 0 ; 

  do {

    /*posiciona no bloco atual*/
    fseek(doc, data_offset + pos, SEEK_SET) ;
    memset(buf, 0, BUFFER_SIZE) ;
    to_read = (data_size - pos > BUFFER_SIZE) ? BUFFER_SIZE : (data_size - pos) ;
    read = fread(buf, 1, to_read, doc) ;

    printf("\n--- Bloco %ld/%ld ---\n", pos / BUFFER_SIZE + 1, (data_size + BUFFER_SIZE - 1) / BUFFER_SIZE) ;
    for (j = 0; j < read; j++)
      putchar(buf[j]) ;
    printf("\n") ;

    if (pos + read >= data_size)
      printf("[Fim do documento]\n") ;
    if (pos == 0)
      printf("[Início do documento]\n") ;

    printf("Comando (n=próximo, p=anterior, q=sair): ") ;
    scanf(" %c", &command) ;

    if (command == 'n') {
      if (pos + read < data_size)
        pos += read ;
      else
        printf("Já está no fim do documento.\n") ; 
    } 
    if (command == 'p') {
      if (pos == 0)
        printf("Já está no início do documento.\n") ;
      else {
        pos -= BUFFER_SIZE ;
        if (pos < 0) pos = 0 ;
      }
    }
    if (command != 'p' && command != 'n' && command != 'q') {
      fclose(doc) ;
      printf("Comando inválido, saindo\n") ;
      return -1 ;
    } 

  } while (command != 'q') ;

  fclose(doc) ;
  return 0 ;
} 

int gbv_order(Library *lib, const char *archive, const char *criteria) {

  Document *aux ;
  FILE *arc, *tmp_arc ;
  char buf[BUFFER_SIZE], *tmp_arc_name;
  int i, j, id ;
  long new_offset, left, n_buf ;
   
  if (!lib || !archive || !criteria) {
    perror("erro gbv_order\n");
    exit(1) ;
  }

  aux = doc_sort(lib, criteria) ; 
  if(!aux) {
    return -1 ;
  }

  arc = fopen(archive, "rb");
  if (!arc) {
    perror("erro na abertura do arquivo\n");
    free(aux) ;
    exit(1) ;
  }

  tmp_arc_name = "tmp_arc" ; 
  tmp_arc = fopen(tmp_arc_name, "wb+");
  if (!tmp_arc) {
    perror("erro ao criar arquivo temporiario\n");
    fclose(arc) ;
    free(aux) ;
    exit(1) ;
  }

  /*escreve a assinatura e o count */
  fwrite("gbva", 4, 1, tmp_arc) ;
  fwrite(&lib->count, 1, sizeof(int), tmp_arc) ;

  /*escreve apenas para ocupar o espaco correto*/
  for (i = 0; i < lib->count; i++)
    fwrite(&lib->docs[i], sizeof(Document), 1, tmp_arc) ;
  
  /*formula um novo arquivo escrevendo ordenadamente*/
  for (i = 0; i < lib->count; i++) {

    /*acha o id antigo do doc para pegar o conteudo na lib*/
    id = -1 ;
    for (j = 0; j < lib->count; j++) {

      if (strcmp(aux[i].name, lib->docs[j].name) == 0) {

        id = j ;
        break ;
      }
    }
    if (id < 0) {

      perror("erro gbv_order\n") ;
      exit(1) ;
    }

    new_offset = ftell(tmp_arc) ;
    aux[i].offset = new_offset ;

    fseek(arc, lib->docs[id].offset, SEEK_SET) ;
    left = aux[i].size ;

    /*calcula quanto falta para escrever*/
    /*caso seja maior que o tamanho do buf pega o restante*/
    while (left > 0) {

      memset(buf, 0, BUFFER_SIZE) ;
      n_buf = (left > BUFFER_SIZE) ? BUFFER_SIZE : left ;
      fread(buf, 1, n_buf, arc) ;
      fwrite(buf, 1, n_buf, tmp_arc) ;
      left -= n_buf ;
    }
  }

  /*reescreve o diretorio no novo archive*/
  fseek(tmp_arc, 8, SEEK_SET) ;
  for (i = 0; i < lib->count; i++)
    fwrite(&aux[i], sizeof(Document), 1, tmp_arc) ;

  fclose(arc) ;
  fclose(tmp_arc) ;

  remove(archive) ;
  rename(tmp_arc_name, archive) ;

  for (i = 0; i < lib->count; i++)
    lib->docs[i] = aux[i] ;

  free(aux) ;
  return 0 ;
} 