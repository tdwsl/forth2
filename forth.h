/* sforth - tdwsl 2022 */

#ifndef FORTH_H
#define FORTH_H

#include <stdbool.h>

#define FORTH_STACK_SIZE 256
#define FORTH_LSTACK_SIZE 128
#define FORTH_ISTACK_SIZE 64
#define FORTH_MEMORY_SIZE 65536

enum {
  FORTH_PUSH,
  FORTH_DROP,
  FORTH_PLUS,
  FORTH_MINUS,
  FORTH_DIV,
  FORTH_MUL,
  FORTH_MOD,
  FORTH_DUP,
  FORTH_OVER,
  FORTH_ROT,
  FORTH_SWAP,
  FORTH_CALL,
  FORTH_JUMP,
  FORTH_JZ,
  FORTH_JNZ,
  FORTH_DO,
  FORTH_LOOP,
  FORTH_DEPTH,
  FORTH_I,
  FORTH_CR,
  FORTH_FULLSTOP,
  FORTH_RECURSE,
  FORTH_LESS,
  FORTH_GREATER,
  FORTH_INC,
  FORTH_DEC,
  FORTH_EQUAL,
  FORTH_PUTSTR,
  FORTH_BYE,
  FORTH_SETMEM,
  FORTH_GETMEM,
  FORTH_HERE,
  FORTH_ALLOT,
};

typedef struct forthWord {
  char *identifier;
  char *program;
  int size;
  char **strings;
  int num_strings;
} ForthWord;

typedef struct forthInstance {
  struct {
    ForthWord *words;
    int size;
    int lock;
  } dict;
  int stack[FORTH_STACK_SIZE];
  int lstack[FORTH_LSTACK_SIZE];
  int sp, lsp;
  bool quit;
  unsigned char memory[FORTH_MEMORY_SIZE];
  int here;
} ForthInstance;

ForthInstance *forth_newInstance();
void forth_freeInstance(ForthInstance *fth);

bool forth_has(ForthInstance *fth, int n);
int forth_pop(ForthInstance *fth);

void forth_push(ForthInstance *fth, int n);

void forth_runWord(ForthInstance *fth, ForthWord w);
void forth_printWord(ForthInstance *fth, ForthWord w);

void forth_runString(ForthInstance *fth, char *text);
void forth_runFile(ForthInstance *fth, const char *filename);

#endif
