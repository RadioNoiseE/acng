/*
 * Read in Cisco AnyConnect `profile.xml' while prompting the user for
 * information and then establish the connection.  Remember to link to expat
 * when compiling.
 *
 * You should pass the path of the profile as the only argument.  Quote when
 * necessary.  It will then prompt you for the host server, your username, and
 * your password.  Your input is NOT being hide so be careful when you are doing
 * this while someone can see your screen.  You can also choose to hardcode the
 * authentication information though discouraged.  Your input is NOT being fully
 * escaped (i.e., `$(command)' still works) before being executed by `system()',
 * which means you should NOT give root execution permission of this binary to
 * non-root users unless you are very clear what you are doing.
 *
 * Helper functions and types are three characters long.  Global (local)
 * variables are two characters long while temporary variables are one
 * characters long.
 *
 * To make parsing faster, you might want to increase the buffer size for input
 * file, however it will have larger memory footprint as the string buffer is
 * created on the stack (not on heap for potentially faster read/write speed).
 *
 * On a POSIX-compliant system, correct width for the host server name is
 * calculated for aligned terminal display.  Undefined behaviour on non-POSIX
 * systems.
 *
 * Copyright (C) 2024, Jing Huang <RadioNoiseE@gmail.com>
 */

#include "expat.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define bs 4096 /* buffer size for input file */
#define cw 12   /* width for each column */
#define cn 5    /* number of column per line */
#define pl 128  /* maximum length for password and username */
#define sc                                                                     \
  "echo \"%s\" | openconnect --protocol=anyconnect --quiet --background "      \
  "--passwd-on-stdin --user=\"%s\" %s >/dev/null 2>&1"

int il = 6;

typedef struct {
  char *ky;
  char *vl;
} svl;

typedef struct {
  int cd, ct, cl;
  svl *sl;
} udh;

int dgt(int n) {
  if (n / 10 == 0)
    return 1;
  return 1 + dgt(n / 10);
}

int uni(const char *s) {
  int l = mbstowcs(NULL, s, 0) + 1, a = 0;
  wchar_t *d = malloc(sizeof(wchar_t) * l);
  mbstowcs(d, s, l);
  a = wcswidth(d, l);
  free(d);
  return a;
}

void XMLCALL ehb(void *d, const char *s, const char **a) {
  udh *ud = d;
  if (ud->cd == 2 && !strcmp(s, "HostEntry")) {
    ud->ct = 1;
    if (ud->cl++ == il)
      ud->sl = realloc(ud->sl, sizeof(svl) * (il *= 2));
  } else if (ud->cd == 3 && ud->ct == 1) {
    if (!strcmp(s, "HostName"))
      ud->ct = 2;
    else if (!strcmp(s, "HostAddress"))
      ud->ct = 3;
  }
  ud->cd++;
}

void XMLCALL ehe(void *d, const char *s) {
  udh *ud = d;
  if (ud->cd != 4)
    ud->ct = 0;
  else
    ud->ct = 1;
  ud->cd--;
}

void XMLCALL cdh(void *d, const XML_Char *s, int l) {
  udh *ud = d;
  if (ud->ct == 2) {
    ud->sl[ud->cl - 1].ky = malloc((l + 1) * sizeof(XML_Char));
    strncpy(ud->sl[ud->cl - 1].ky, s, l);
    ud->ct = 1;
  } else if (ud->ct == 3) {
    ud->sl[ud->cl - 1].vl = malloc((l + 1) * sizeof(XML_Char));
    strncpy(ud->sl[ud->cl - 1].vl, s, l);
    ud->ct = 1;
  }
}

int main(int ac, char **as) {
  if (ac < 2) {
    fprintf(stderr, "%s: Missing input file\n", *as);
    goto die;
  } else if (ac >= 3) {
    fprintf(stderr, "%s: Too many options\n", *as);
    goto die;
  }

  FILE *px = fopen(*++as, "r");
  if (!px) {
    fprintf(stderr, "fopen: Can't open %s\n", *as);
    goto die;
  }

  XML_Parser ph = XML_ParserCreate("UTF-8");
  if (!ph) {
    fprintf(stderr, "expat: Can't create xml parser\n");
    goto die;
  }

  udh ud = {0};
  ud.sl = malloc(il * sizeof(svl));
  XML_SetUserData(ph, &ud);
  XML_SetElementHandler(ph, ehb, ehe);
  XML_SetCharacterDataHandler(ph, cdh);

  printf("%s: Extracting host servers\n", *--as);
  fflush(stdout);
  setlocale(LC_ALL, "");

  int f, l;
  char b[bs], u[pl + 1], p[pl + 1];

  do {
    f = (l = fread(b, sizeof(char), sizeof(b), px)) < sizeof(b);
    if (XML_Parse(ph, b, l, f) == XML_STATUS_ERROR)
      fprintf(stderr, "expat: Invalid input %s at line %ld\n",
              XML_ErrorString(XML_GetErrorCode(ph)),
              XML_GetErrorLineNumber(ph));
  } while (!f);

  l = dgt(ud.cl - 1);
  while (ud.cd++ < ud.cl) {
    int p = cw - uni(ud.sl[ud.cd - 1].ky);
    printf("[%*d] %s", l, ud.cd - 1, ud.sl[ud.cd - 1].ky);
    if (!(ud.cd % cn))
      printf("\n");
    else
      for (int i = 0; i < p; i++)
        printf(" ");
    free(ud.sl[ud.cd - 1].ky);
  }

  if ((ud.cd - 1) % cn)
    printf("\n");

  printf("Server: ");
  scanf("%d", &ud.cd);

  printf("User: ");
  scanf("%s", u);

  printf("Key: ");
  scanf("%s", p);

  if (ud.cd <= ud.cl) {
    l = strlen(sc) + strlen(ud.sl[ud.cd].vl) + strlen(u) + strlen(p) + 1;
    char *c = malloc(l * sizeof(char));
    snprintf(c, l, sc, p, u, ud.sl[ud.cd].vl);
    if ((l = system(c)) != 0)
      fprintf(stderr, "%s: System call failed with return code %d\n", *as, l);
    else
      printf("%s: Connection established\n", *as);
    free(c);
  } else
    fprintf(stderr, "%s: Unknown server\n", *as);

  XML_ParserFree(ph);
  while (ud.cl-- > 0)
    free(ud.sl[ud.cl].vl);
  free(ud.sl);

  return 0;

die:
  return -1;
}
