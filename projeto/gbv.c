#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gbv.h" 

int lib_insert(const char* nome_arquivo, long posicao, const void* dados,
               size_t tamanho_dados) {

  FILE* arquivo = fopen(nome_arquivo, "rb+");
  if (arquivo == NULL) {
    printf("Erro ao abrir o arquivo.\n");
    return -1;
  }

  // Obter tamanho do arquivo
  fseek(arquivo, 0, SEEK_END);
  long tamanho_total = ftell(arquivo);
  
  if (posicao < 0 || posicao > tamanho_total) {
    printf("Posição inválida.\n");
    fclose(arquivo);
    return -1;
  }

  if (tamanho_dados == 0) {
    fclose(arquivo);
    return 0;
  }

  // Expandir o arquivo
  fseek(arquivo, 0, SEEK_END);
  for (long i = 0; i < tamanho_dados; i++) {
    fputc(0, arquivo);
  }

  // Deslocar dados do final para o início
  unsigned char buffer[BUFFER_SIZE];
  long bytes_restantes = tamanho_total - posicao;
  long offset_origem, offset_destino;
  
  while (bytes_restantes > 0) {
    // Calcular tamanho do próximo bloco a mover
    size_t bloco_size = (bytes_restantes > BUFFER_SIZE) ? BUFFER_SIZE : bytes_restantes;
    
    // Calcular offsets
    offset_origem = posicao + bytes_restantes - bloco_size;
    offset_destino = offset_origem + tamanho_dados;
    
    // Ler bloco
    fseek(arquivo, offset_origem, SEEK_SET);
    size_t lidos = fread(buffer, 1, bloco_size, arquivo);
    
    if (lidos == 0) break;
    
    // Escrever bloco
    fseek(arquivo, offset_destino, SEEK_SET);
    fwrite(buffer, 1, lidos, arquivo);
    
    bytes_restantes -= lidos;
  }

  // Escrever novos dados
  fseek(arquivo, posicao, SEEK_SET);
  fwrite(dados, 1, tamanho_dados, arquivo);

  fclose(arquivo);
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

  // Aloca o vetor de documentos
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

int gbv_add(Library *lib, const char *archive, const char *docname) {
  FILE *farc, *fdoc;
  Document doc;
  struct stat data;
  char buf[BUFFER_SIZE];
  size_t n_buf;

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
  if (!farc) {
    perror("erro na abertura do arquivo\n");
    exit(1);
  }
  fdoc = fopen(docname, "rb");
  if (!fdoc) {
    perror("erro na abertura do documento\n");
    fclose(farc);
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

  // Insere a estrutura Document no diretório (logo após o cabeçalho)
  long dir_pos = 8 + lib->count * sizeof(Document); // 4 bytes sig + 4 bytes count
  lib_insert(archive, dir_pos, &doc, sizeof(Document));

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
  int new_count = lib->count + 1;
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

