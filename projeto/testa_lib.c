#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gbv.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Uso: %s <arquivo_biblioteca>\n", argv[0]);
    return 1;
  }

  FILE *f = fopen(argv[1], "rb");
  if (!f) {
    perror("Erro ao abrir arquivo");
    return 1;
  }

  // Ler assinatura
  char sig[5] = {0};
  fread(sig, 1, 4, f);
  printf("Assinatura: %s\n", sig);

  // Ler quantidade de documentos
  int count = 0;
  fread(&count, sizeof(int), 1, f);
  printf("Quantidade de documentos: %d\n", count);

  // Ler e imprimir cada Document
  for (int i = 0; i < count; i++) {
    Document doc;
    fread(&doc, sizeof(Document), 1, f);
    printf("\nDocumento %d:\n", i + 1);
    printf("  Nome: %s\n", doc.name);
    printf("  Tamanho: %ld bytes\n", doc.size);
    printf("  Data: %s", ctime(&doc.date));
    printf("  Offset: %ld\n", doc.offset);
  }

  // Ler e imprimir o conteúdo como texto
  printf("\n--- Conteúdo dos documentos (como texto) ---\n");
  fseek(f, 0, SEEK_END);
  long end = ftell(f);
  for (int i = 0; i < count; i++) {
    // Volta para o início do arquivo para reler os Document
    fseek(f, 8 + i * sizeof(Document), SEEK_SET);
    Document doc;
    fread(&doc, sizeof(Document), 1, f);

    if (doc.offset + doc.size > end) {
      printf("Conteúdo fora do arquivo!\n");
      continue;
    }

    char *conteudo = malloc(doc.size + 1);
    if (!conteudo) {
      printf("Erro de memória!\n");
      continue;
    }
    fseek(f, doc.offset, SEEK_SET);
    fread(conteudo, 1, doc.size, f);
    conteudo[doc.size] = '\0';
    printf("\n[%s]:\n%s\n", doc.name, conteudo);
    free(conteudo);
  }

  fclose(f);
  return 0;
}
