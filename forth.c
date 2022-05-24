/* forth implementation - tdwsl 2022 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define FORTH_STACK_SIZE 256
#define FORTH_LSTACK_SIZE 128
#define FORTH_ISTACK_SIZE 64

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
};

typedef struct forthWord {
  char *identifier;
  char *program;
  int size;
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
} ForthInstance;

const char *forth_compileOnly[] = {
  "IF",
  "THEN",
  "ELSE",
  "DO",
  "LOOP",
  "I",
  "RECURSE",
  0,
};

void forth_uppercase(char *s) {
  for(char *c = s; *c; c++)
    if(*c >= 'a' && *c <= 'z')
      *c = *c - 'a' + 'A';
}

int forth_chars2int(char *c) {
  int n = c[0] << 24;
  n |= c[1] << 16;
  n |= c[2] << 8;
  n |= c[3];
}

void forth_int2chars(int n, char *c) {
  c[0] = n>>24;
  c[1] = n>>16;
  c[2] = n>>8;
  c[3] = n;
}

bool forth_isnum(char *s, int *n) {
  if(!strlen(s))
    return false;
  bool neg = false;
  *n = 0;
  for(char *c = s; *c; c++) {
    if(*c == '-') {
      if(c != s || *(c+1) == 0)
        return false;
      else
        neg = true;
    }
    else if(*c >= '0' && *c <= '9')
      *n = (*n)*10 + (*c) - '0';
    else
      return false;
  }
  if(neg)
    (*n) *= -1;
  return true;
}

void forth_initWord(ForthWord *w, char *identifier) {
  w->identifier = malloc(strlen(identifier)+1);
  strcpy(w->identifier, identifier);
  forth_uppercase(w->identifier);
  w->program = 0;
  w->size = 0;
}

void forth_addInstruction(ForthWord *w, char ins) {
  w->program = realloc(w->program, ++(w->size));
  w->program[w->size-1] = ins;
}

void forth_addInteger(ForthWord *w, int n) {
  w->size += 4;
  w->program = realloc(w->program, w->size);
  forth_int2chars(n, w->program+w->size-4);
}

void forth_concatWord(ForthWord *w, ForthWord w2) {
  w->program = realloc(w->program, w->size+w2.size);
  for(int i = 0; i < w2.size; i++)
    w->program[w->size+i] = w2.program[i];
  w->size += w2.size;
}

void forth_addWord(ForthInstance *fth, ForthWord w) {
  fth->dict.words = realloc(fth->dict.words,
      sizeof(ForthWord)*(++(fth->dict.size)));
  fth->dict.words[fth->dict.size-1] = w;
}

void forth_addDefaultWords(ForthInstance *fth) {
  ForthWord w;

  forth_initWord(&w, "+");
  forth_addInstruction(&w, FORTH_PLUS);
  forth_addWord(fth, w);

  forth_initWord(&w, "-");
  forth_addInstruction(&w, FORTH_MINUS);
  forth_addWord(fth, w);

  forth_initWord(&w, "/");
  forth_addInstruction(&w, FORTH_DIV);
  forth_addWord(fth, w);

  forth_initWord(&w, "*");
  forth_addInstruction(&w, FORTH_MUL);
  forth_addWord(fth, w);

  forth_initWord(&w, "MOD");
  forth_addInstruction(&w, FORTH_MOD);
  forth_addWord(fth, w);

  forth_initWord(&w, "DUP");
  forth_addInstruction(&w, FORTH_DUP);
  forth_addWord(fth, w);

  forth_initWord(&w, "OVER");
  forth_addInstruction(&w, FORTH_OVER);
  forth_addWord(fth, w);

  forth_initWord(&w, "ROT");
  forth_addInstruction(&w, FORTH_ROT);
  forth_addWord(fth, w);

  forth_initWord(&w, "SWAP");
  forth_addInstruction(&w, FORTH_SWAP);
  forth_addWord(fth, w);

  forth_initWord(&w, "DROP");
  forth_addInstruction(&w, FORTH_DROP);
  forth_addWord(fth, w);

  forth_initWord(&w, "DEPTH");
  forth_addInstruction(&w, FORTH_DEPTH);
  forth_addWord(fth, w);

  forth_initWord(&w, ".");
  forth_addInstruction(&w, FORTH_FULLSTOP);
  forth_addWord(fth, w);

  forth_initWord(&w, "CR");
  forth_addInstruction(&w, FORTH_CR);
  forth_addWord(fth, w);

  forth_initWord(&w, "<");
  forth_addInstruction(&w, FORTH_LESS);
  forth_addWord(fth, w);

  forth_initWord(&w, ">");
  forth_addInstruction(&w, FORTH_GREATER);
  forth_addWord(fth, w);

  forth_initWord(&w, "1+");
  forth_addInstruction(&w, FORTH_INC);
  forth_addWord(fth, w);

  forth_initWord(&w, "1-");
  forth_addInstruction(&w, FORTH_DEC);
  forth_addWord(fth, w);

  forth_initWord(&w, "<=");
  forth_addInstruction(&w, FORTH_DEC);
  forth_addInstruction(&w, FORTH_LESS);
  forth_addWord(fth, w);

  forth_initWord(&w, ">=");
  forth_addInstruction(&w, FORTH_INC);
  forth_addInstruction(&w, FORTH_GREATER);
  forth_addWord(fth, w);

  forth_initWord(&w, "=");
  forth_addInstruction(&w, FORTH_EQUAL);
  forth_addWord(fth, w);

  fth->dict.lock = fth->dict.size;
}

ForthInstance *forth_newInstance() {
  ForthInstance *fth = malloc(sizeof(ForthInstance));
  fth->sp = 0;
  fth->lsp = 0;
  fth->dict.size = 0;
  fth->dict.words = 0;
  fth->dict.lock = 0;
  forth_addDefaultWords(fth);
  return fth;
}

void forth_freeInstance(ForthInstance *fth) {
  for(int i = 0; i < fth->dict.size; i++) {
    free(fth->dict.words[i].identifier);
    if(fth->dict.words[i].program)
      free(fth->dict.words[i].program);
  }
  if(fth->dict.words)
    free(fth->dict.words);
  free(fth);
}

bool forth_has(ForthInstance *fth, int n) {
  if(fth->sp >= n)
    return true;
  else {
    printf("stack underflow !\n");
    return false;
  }
}

int forth_pop(ForthInstance *fth) {
  if(forth_has(fth, 1))
    return fth->stack[--(fth->sp)];
  else
    return 0;
}

void forth_push(ForthInstance *fth, int n) {
  if(fth->sp >= FORTH_STACK_SIZE)
    printf("stack overflow !\n");
  else
    fth->stack[fth->sp++] = n;
}

char **forth_splitString(char *text) {
  char **strings = 0;
  int num_strings = 0;

  int max = 50;
  const int buf = 20;
  char *s = malloc(max);
  int len = 0;

  bool comment = false;
  char quote = 0;

  for(char *c = text; ; c++) {
    if(*c == '\n' || *c == 0) {
      s[len] = 0;
      len = 0;

      if(s[0] || quote) {
        strings = realloc(strings, sizeof(char*)*(++num_strings));
        strings[num_strings-1] = malloc(strlen(s)+1);
        strcpy(strings[num_strings-1], s);
        if(!quote)
          forth_uppercase(strings[num_strings-1]);
      }

      comment = false;
      quote = 0;

      if(*c == 0)
        break;

      continue;
    }

    if(comment)
      continue;

    if(quote) {
      if(*c == quote) {
        s[len] = 0;
        len = 0;
        strings = realloc(strings, sizeof(char*)*(++num_strings));
        strings[num_strings-1] = malloc(strlen(s)+1);
        strcpy(strings[num_strings-1], s);
        quote = 0;
      }
      else {
        s[len++] = *c;
        if(len > max-buf) {
          max += buf;
          s = realloc(s, max);
        }
      }
      continue;
    }

    if(*c == '\t' || *c == ' ') {
      if(!len)
        continue;

      s[len] = 0;
      len = 0;

      strings = realloc(strings, sizeof(char*)*(++num_strings));
      strings[num_strings-1] = malloc(strlen(s)+1);
      strcpy(strings[num_strings-1], s);
      forth_uppercase(strings[num_strings-1]);

      continue;
    }

    s[len++] = *c;
    if(len > max-buf) {
      max += buf;
      s = realloc(s, max);
    }
  }

  free(s);

  strings = realloc(strings, sizeof(char*)*(++num_strings));
  strings[num_strings-1] = 0;

  return strings;
}

void forth_runWord(ForthInstance *fth, ForthWord w) {
  int pc = 0;
  int n1, n2, n3;
  while(pc < w.size)
    switch(w.program[pc++]) {
    case FORTH_PUSH:
      forth_push(fth, forth_chars2int(w.program+pc));
      pc += 4;
      break;
    case FORTH_PLUS:
      forth_push(fth, forth_pop(fth)+forth_pop(fth));
      break;
    case FORTH_MUL:
      forth_push(fth, forth_pop(fth)*forth_pop(fth));
      break;
    case FORTH_MINUS:
      n1 = forth_pop(fth);
      forth_push(fth, forth_pop(fth)-n1);
      break;
    case FORTH_DIV:
      n1 = forth_pop(fth);
      forth_push(fth, forth_pop(fth)/n1);
      break;
    case FORTH_MOD:
      n1 = forth_pop(fth);
      forth_push(fth, forth_pop(fth)%n1);
      break;
    case FORTH_DUP:
      n1 = forth_pop(fth);
      forth_push(fth, n1);
      forth_push(fth, n1);
      break;
    case FORTH_SWAP:
      if(forth_has(fth, 2)) {
        n1 = fth->stack[fth->sp-1];
        fth->stack[fth->sp-1] = fth->stack[fth->sp-2];
        fth->stack[fth->sp-2] = n1;
      }
      break;
    case FORTH_DROP:
      forth_pop(fth);
      break;
    case FORTH_ROT:
      if(forth_has(fth, 3)) {
        n1 = fth->stack[fth->sp-1];
        fth->stack[fth->sp-1] = fth->stack[fth->sp-3];
        fth->stack[fth->sp-3] = fth->stack[fth->sp-2];
        fth->stack[fth->sp-2] = n1;
      }
      break;
    case FORTH_OVER:
      if(forth_has(fth, 2))
        forth_push(fth, fth->stack[fth->sp-2]);
      break;
    case FORTH_DEPTH:
      forth_push(fth, fth->sp);
      break;
    case FORTH_FULLSTOP:
      printf("%d ", forth_pop(fth));
      break;
    case FORTH_CR:
      printf("\n");
      break;
    case FORTH_CALL:
      n1 = forth_chars2int(w.program+pc);
      forth_runWord(fth, fth->dict.words[n1]);
      pc += 4;
      break;
    case FORTH_RECURSE:
      forth_runWord(fth, w);
      break;
    case FORTH_JUMP:
      pc = forth_chars2int(w.program+pc);
      break;
    case FORTH_JZ:
      if(!forth_pop(fth))
        pc = forth_chars2int(w.program+pc);
      else
        pc += 4;
      break;
    case FORTH_JNZ:
      if(forth_pop(fth))
        pc = forth_chars2int(w.program+pc);
      else
        pc += 4;
      break;
    case FORTH_DO:
      fth->lstack[fth->lsp++] = forth_pop(fth);
      fth->lstack[fth->lsp++] = forth_pop(fth);
      break;
    case FORTH_LOOP:
      n1 = fth->lstack[fth->lsp-1];
      fth->lstack[fth->lsp-2]++;
      n2 = fth->lstack[fth->lsp-2];
      if(n2 < n1)
        pc = forth_chars2int(w.program+pc);
      else {
        fth->lsp -= 2;
        pc += 4;
      }
      break;
    case FORTH_I:
      forth_push(fth, fth->lstack[fth->lsp-2]);
      break;
    case FORTH_INC:
      if(forth_has(fth, 1))
        fth->stack[fth->sp-1]++;
      break;
    case FORTH_DEC:
      if(forth_has(fth, 1))
        fth->stack[fth->sp-1]--;
      break;
    case FORTH_GREATER:
      n1 = forth_pop(fth);
      n2 = forth_pop(fth);
      forth_push(fth, n2 > n1);
      break;
    case FORTH_LESS:
      n1 = forth_pop(fth);
      n2 = forth_pop(fth);
      forth_push(fth, n2 < n1);
      break;
    case FORTH_EQUAL:
      forth_push(fth, forth_pop(fth) == forth_pop(fth));
      break;
    }
}

void forth_printWord(ForthInstance *fth, ForthWord w) {
  int pc = 0;
  printf("%s:\n", w.identifier);
  while(pc < w.size) {
    printf("%d\t", pc);
    switch(w.program[pc++]) {
    case FORTH_PLUS:
      printf("+"); break;
    case FORTH_MINUS:
      printf("-"); break;
    case FORTH_DIV:
      printf("/"); break;
    case FORTH_MUL:
      printf("*"); break;
    case FORTH_MOD:
      printf("MOD"); break;
    case FORTH_PUSH:
      printf("push"); break;
    case FORTH_DROP:
      printf("DROP"); break;
    case FORTH_DO:
      printf("DO"); break;
    case FORTH_LOOP:
      printf("LOOP"); break;
    case FORTH_JZ:
      printf("jz"); break;
    case FORTH_JNZ:
      printf("jnz"); break;
    case FORTH_JUMP:
      printf("jump"); break;
    case FORTH_CR:
      printf("CR"); break;
    case FORTH_FULLSTOP:
      printf("."); break;
    case FORTH_CALL:
      printf("call"); break;
    case FORTH_RECURSE:
      printf("RECURSE"); break;
    case FORTH_SWAP:
      printf("SWAP"); break;
    case FORTH_DUP:
      printf("DUP"); break;
    case FORTH_OVER:
      printf("OVER"); break;
    case FORTH_ROT:
      printf("ROT"); break;
    case FORTH_I:
      printf("I"); break;
    case FORTH_INC:
      printf("1+"); break;
    case FORTH_DEC:
      printf("1-"); break;
    case FORTH_GREATER:
      printf(">"); break;
    case FORTH_LESS:
      printf("<"); break;
    case FORTH_EQUAL:
      printf("="); break;
    case FORTH_DEPTH:
      printf("DEPTH"); break;
    }
    switch(w.program[pc-1]) {
    default:
      break;
    case FORTH_LOOP:
    case FORTH_JNZ:
    case FORTH_JZ:
    case FORTH_JUMP:
    case FORTH_PUSH:
      printf(" %d", forth_chars2int(w.program+pc));
      pc += 4;
      break;
    case FORTH_CALL:
      printf(" %s",
          fth->dict.words[forth_chars2int(w.program+pc)].identifier);
      pc += 4;
      break;
    }
    printf("\n");
  }
}

void forth_callWord(ForthInstance *fth, char *string) {
  int n;
  if(forth_isnum(string, &n))
    forth_push(fth, n);

  else {
    for(int i = 0; i < fth->dict.size; i++)
      if(strcmp(fth->dict.words[i].identifier, string) == 0) {
        forth_runWord(fth, fth->dict.words[i]);
        return;
      }

    for(int i = 0; forth_compileOnly[i]; i++)
      if(strcmp(string, forth_compileOnly[i]) == 0) {
        printf("%s is compile only !\n");
        return;
      }

    printf("%s ?\n", string);
  }
}

void forth_runString(ForthInstance *fth, char *text) {
  bool compile = false;
  int if_a[FORTH_ISTACK_SIZE];
  int else_a[FORTH_ISTACK_SIZE];
  int if_sp = 0;
  int do_a[FORTH_LSTACK_SIZE];
  int do_sp = 0;
  char **strings = forth_splitString(text);
  ForthWord w;
  int taken;

  for(int i = 0; strings[i]; i++) {
    char *string = strings[i];

    if(compile) {
      int n;
      if(forth_isnum(string, &n)) {
        forth_addInstruction(&w, FORTH_PUSH);
        forth_addInteger(&w, n);
      }

      else if(strcmp(string, ";") == 0) {
        if(if_sp) {
          printf("expect THEN after IF in %s\n", w.identifier);
          free(w.program);
          free(w.identifier);
        }
        else if(do_sp) {
          printf("expect LOOP after DO in %s\n", w.identifier);
          free(w.program);
          free(w.identifier);
        }

        else if(taken == -1)
          forth_addWord(fth, w);
        else {
          free(fth->dict.words[taken].identifier);
          if(fth->dict.words[taken].program)
            free(fth->dict.words[taken].program);

          fth->dict.words[taken] = w;
        }

        compile = false;
      }

      else if(strcmp(string, "IF") == 0) {
        forth_addInstruction(&w, FORTH_JZ);
        else_a[if_sp] = -1;
        if_a[if_sp++] = w.size;
        forth_addInteger(&w, 0);
      }
      else if(strcmp(string, "ELSE") == 0) {
        forth_addInstruction(&w, FORTH_JUMP);
        else_a[if_sp-1] = w.size;
        forth_addInteger(&w, 0);
      }
      else if(strcmp(string, "THEN") == 0) {
        if(if_sp <= 0) {
          printf("expect IF before THEN in %s\n", w.identifier);
          continue;
        }

        if_sp--;
        if(else_a[if_sp] == -1) {
          forth_int2chars(else_a[if_sp]+4, w.program+if_a[if_sp]);
          forth_int2chars(w.size, w.program+else_a[if_sp]);
        }
        else
          forth_int2chars(w.size, w.program+if_a[if_sp]);
      }

      else if(strcmp(string, "RECURSE") == 0)
        forth_addInstruction(&w, FORTH_RECURSE);

      else if(strcmp(string, "DO") == 0) {
        forth_addInstruction(&w, FORTH_DO);
        do_a[do_sp++] = w.size;
      }
      else if(strcmp(string, "LOOP") == 0) {
        if(do_sp <= 0) {
          printf("expect DO before LOOP in %s\n", w.identifier);
          continue;
        }

        forth_addInstruction(&w, FORTH_LOOP);
        forth_addInteger(&w, do_a[--do_sp]);
      }
      else if(strcmp(string, "I") == 0)
        forth_addInstruction(&w, FORTH_I);

      else {
        bool found = false;
        for(int i = 0; i < fth->dict.lock; i++)
          if(strcmp(string, fth->dict.words[i].identifier) == 0) {
            forth_concatWord(&w, fth->dict.words[i]);
            found = true;
            break;
          }
        if(found)
          continue;

        found = false;
        for(int i = fth->dict.lock; i < fth->dict.size; i++)
          if(strcmp(string, fth->dict.words[i].identifier) == 0) {
            forth_addInstruction(&w, FORTH_CALL);
            forth_addInteger(&w, i);
            found = true;
            break;
          }
        if(found)
          continue;

        printf("%s ?\n", string);
      }
    }

    else {
      if(strcmp(string, ":") == 0) {
        i++;
        string = strings[i];

        if(string == 0) {
          printf("expect identifier after :\n");
          continue;
        }
        if(strcmp(string, ";") == 0)
          continue;

        int n;
        if(forth_isnum(string, &n)) {
          printf("identifier cannot be number !\n");
          continue;
        }

        bool found = false;
        for(int j = 0; j < fth->dict.lock; j++)
          if(strcmp(string, fth->dict.words[j].identifier) == 0) {
            printf("cannot redefine %s !\n");
            found = true;
            break;
          }
        if(found)
          continue;

        taken = -1;

        for(int j = fth->dict.lock; j < fth->dict.size; j++)
          if(strcmp(string, fth->dict.words[j].identifier) == 0) {
            taken = j;
            break;
          }

        forth_initWord(&w, string);
        compile = true;
      }

      else
        forth_callWord(fth, string);
    }
  }

  if(compile) {
    printf("expect ; after : in %s\n", w.identifier);
    free(w.identifier);
    free(w.program);
  }

  for(int i = 0; strings[i]; i++)
    free(strings[i]);
  free(strings);
}

int main() {
  ForthInstance *fth = forth_newInstance();
  forth_runString(fth, "1 2 + . cr : multiples 11 1 do dup i * . loop cr drop ; 7 multiples");
  forth_printWord(fth, fth->dict.words[fth->dict.lock]);
  forth_freeInstance(fth);
  return 0;
}
