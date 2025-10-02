#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gbv.h" 

/*move o arc criando um espaço de data_size bytes na posicao pos*/
int lib_move(const char* arc_name, long pos, long data_size) {

  FILE* arc = fopen(arc_name, "rb+");
  if (arc == NULL) {
    printf("Erro ao abrir o arquivo.\n");
    return -1;
  }

  // Obter tamanho do arquivo
  fseek(arc, 0, SEEK_END);
  long arc_size = ftell(arc);
  
  if (pos < 0 || pos > arc_size) {
    printf("Posição inválida.\n");
    fclose(arc);
    return -1;
  }

  if (data_size == 0) {
    fclose(arc);
    return 0;
  }

  // Expandir o arquivo
  fseek(arc, 0, SEEK_END);
  for (long i = 0; i < data_size; i++) {
    fputc(0, arc);
  }

  // Deslocar dados do final para posiçao 
  unsigned char buffer[BUFFER_SIZE];
  long bytes_left = arc_size - pos;
  long offset_orig, offset_dest;
  long bloco_size, read ;
  
  while (bytes_left > 0) {
    // Calcular tamanho do próximo bloco a mover
    bloco_size = (bytes_left > BUFFER_SIZE) ? BUFFER_SIZE : bytes_left;
    
    // Calcular offsets
    offset_orig = pos + bytes_left - bloco_size;
    offset_dest = offset_orig + data_size;
    
    // Ler bloco
    fseek(arc, offset_orig, SEEK_SET);
    read = fread(buffer, 1, bloco_size, arc);
    
    if (read == 0) break;
    
    // Escrever bloco
    fseek(arc, offset_dest, SEEK_SET);
    fwrite(buffer, 1, read, arc);
    
    bytes_left -= read;
  }

  if (arc_size + data_size < arc_size) {
    truncate(arc_name, arc_size + data_size) ;
  }


  fclose(arc);
  return 0;
}

/*escreve o arquivo f dentro do arquivo arc*/
/*funçao sem checagem de sobreescrita ou atualizacao de offset*/
int lib_write(const char *arc_name, long pos, const char *f_name) {

  FILE *arc = fopen(arc_name, "rb+");
  FILE *f = fopen(f_name, "rb") ;
  if (arc == NULL || f == NULL) {
    printf("Erro ao abrir o arquivo.\n");
    return -1;
  }

  // obter tamanho dos arquivos
  fseek(arc, 0, SEEK_END);
  long arc_size = ftell(arc);

  fseek(f, 0, SEEK_END);
  long f_size = ftell(f);

  if (pos < 0 || pos > arc_size) {
    printf("Posição inválida.\n");
    fclose(arc);
    fclose(f);
    return -1;
  }

  if (f_size == 0) {
    fclose(arc);
    fclose(f);
    return 0;
  }

  // escrever conteudo 
  unsigned char buf[BUFFER_SIZE];
  int n_buf ;

  fseek(arc, pos, SEEK_SET) ;

  while ((n_buf = fread(buf, 1, BUFFER_SIZE, f)) > 0) {
    fwrite(buf, 1, n_buf, arc);
  }

  fclose(arc);
  fclose(f);
  return 0;
}

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

  /*testa se é uma Lib*/
  fread(sig, 1, 4, f) ;
  if (strcmp(sig, "gbva") != 0) {
    perror("arquivo não é uma biblioteca\n") ;
    exit(1) ;
  }

  /*montagem da lib*/
  fread(&lib->count, 1, 4, f) ; 

  // aloca o vetor de documentos
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

  FILE *farc, *fdoc;
  Document doc;
  struct stat data;
  char buf[BUFFER_SIZE];
  size_t n_buf;
  int new_count ; 
  long dir_pos ;

  if (!lib || !archive || !docname) {
    perror("erro gbv_add\n");
    exit(1);
  }

  printf("-------%s\n", docname) ;

  // Verifica duplicidade
  for (int i = 0; i < lib->count; i++) {
    if (strcmp(lib->docs[i].name, docname) == 0) {
      printf("Documento já existe na biblioteca.\n");
      return -1;
    }
  }

  farc = fopen(archive, "rb+");
  fdoc = fopen(docname, "rb");
  if (!farc || !fdoc) {
    perror("erro na abertura do arquivo\n");
    exit(1);
  }

  // Preenche a estrutura do documento
  stat(docname, &data);
  strncpy(doc.name, docname, MAX_NAME -1);
  doc.name[MAX_NAME - 1] = '\0';
  doc.date = time(NULL);
  fseek(fdoc, 0, SEEK_END);
  doc.size = ftell(fdoc);
  fseek(fdoc, 0, SEEK_SET);

  // move e insere a estrutura Document no diretório
  dir_pos = 8 + lib->count * sizeof(Document); // 4 bytes sig + 4 bytes count
  lib_move(archive, dir_pos, sizeof(Document));
  fseek(farc, dir_pos, SEEK_SET);
  fwrite(&doc, 1, sizeof(Document), farc);

  // Escreve o conteúdo do documento no final do arquivo da biblioteca
  fseek(farc, 0, SEEK_END);
  doc.offset = ftell(farc);

  while ((n_buf = fread(buf, 1, BUFFER_SIZE, fdoc)) > 0) {
    fwrite(buf, 1, n_buf, farc);
  }
  fclose(fdoc);

  for (int i = 0; i < lib->count; i++) {
    lib->docs[i].offset += sizeof(Document);
    /*atualiza o arquivo*/ 
    fseek(farc, 8 + i * sizeof(Document), SEEK_SET);
    fwrite(&lib->docs[i], sizeof(Document), 1, farc);
  }

  // Corrijir o offset do novo Document 
  fseek(farc, dir_pos, SEEK_SET);
  fwrite(&doc, sizeof(Document), 1, farc);

  // Atualiza o contador de documentos no cabeçalho
  new_count = lib->count + 1;
  fseek(farc, 4, SEEK_SET); // 4 bytes após a assinatura
  fwrite(&new_count, sizeof(int), 1, farc);

  fclose(farc);

  // Atualiza o diretório em memória
  Document *tmp = realloc(lib->docs, new_count * sizeof(Document));
  if (!tmp) {
    perror("erro realloc");
    return -1;
  }
  lib->docs = tmp;
  lib->docs[lib->count] = doc;
  lib->count = new_count;

  return 0;
}

int gbv_remove(Library *lib, const char *docname){

  if (!lib || !docname) {
    perror("erro gbv_remove\n");
    exit(1);
  }
  FILE *arc = fopen(docname, "rb+");
  if (!arc) {
    perror("erro na abertura do arquivo\n");
    exit(1);
  }

  /*acha o documento*/
  /*da realoc no lib->docs*/

  return 0;
} 

/*funcao pronta*/
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

int gbv_view(const Library *lib, const char *docname) {

  int i, n_buf ;
  char command, *buf[BUFFER_SIZE] ;

  if (!lib || !docname) {
    perror("erro gbv_view\n");
    exit(1);
  }
  FILE *doc = fopen(docname, "rb");
  if (!doc) {
    perror("erro na abertura do arquivo\n");
    exit(1);
  }

  /*acha o documento no vetor lib->docs*/
  for (i = 0; i < lib->count; i++) {
    if (strcmp(lib->docs[i].name, docname) == 0)
      break ;
    if (i == lib->count -1) {
      perror("erro gbv_view\n") ;
      fclose(doc) ;
      exit(1) ;
    } 
  }

  /*loop: le o comando*/
  /*le o documento e exibe na tela*/ 
  do {
    
    while ((n_buf = fread(buf, 1, BUFFER_SIZE, doc)) > 0) {
      printf("\n%s\n", buf) ;
    }

    scanf("%c", &command) ;

  }while(command == 'q') ;


  return 0;
} 


int gbv_order(Library *lib, const char *archive, const char *criteria) {
  return 0;
} 

