/* sforth - tdwsl 2022 */

#include <stdio.h>
#include <stdlib.h>
#include "forth.h"

int main(int argc, char **args) {
  if(argc > 2) {
    printf("usage: %s <file>\n", args[0]);
    return 0;
  }

  ForthInstance *fth = forth_newInstance();

  if(argc == 2) {
    forth_runFile(fth, args[1]);
    forth_freeInstance(fth);
    return 0;
  }

  printf("sforth forth interpeter - tdwsl 2022\n");
  printf("type bye to quit\n");

  while(!fth->quit) {
    /* read line */
    int max = 40;
    const int buf = 10;
    char *s = malloc(max);
    int len = 0;

    for(s[len++] = getc(stdin); s[len-1] != '\n'; s[len++] = getc(stdin)) {
      if(s[len-1] == EOF || s[len-1] == 0) {
        fth->quit = true;
        break;
      }
      if(len > max-buf) {
        max += buf;
        s = realloc(s, max);
      }
    }
    s[len-1] = 0;

    /* run line */
    forth_runString(fth, s);
    free(s);

    /* ok */
    if(!fth->quit)
      printf("    ok\n");
  }

  forth_freeInstance(fth);

  return 0;
}
