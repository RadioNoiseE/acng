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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define bs 4096 /* buffer size for input file */
#define cw 12   /* width for each column */
#define cn 5    /* number of column per line */
#define pl 128  /* maximum length for password and username */
#define sc                                                                     \
  "echo \"%s\" | /opt/pkg/sbin/openconnect --user=\"%s\" --passwd-on-stdin "   \
  "--non-inter --syslog --reconnect-timeout=10 %s >/dev/null 2>&1"

int il = 6;

typedef struct {
  char *ky;
  char *vl;
} svl;

typedef struct {
  int cd, ct, cl;
  svl *sl;
} udh;

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

int main() {
  FILE *px = fopen("/Users/rne/.proxy", "r");
  if (!px)
    goto die;

  XML_Parser ph = XML_ParserCreate("UTF-8");
  if (!ph)
    goto die;

  udh ud = {0};
  ud.sl = malloc(il * sizeof(svl));
  XML_SetUserData(ph, &ud);
  XML_SetElementHandler(ph, ehb, ehe);
  XML_SetCharacterDataHandler(ph, cdh);

  int f, l;
  char b[bs], u[pl + 1], p[pl + 1];

  do {
    f = (l = fread(b, sizeof(char), sizeof(b), px)) < sizeof(b);
    if (XML_Parse(ph, b, l, f) == XML_STATUS_ERROR)
      goto die;
  } while (!f);

  FILE *pc = fopen("/Users/rne/.cisco", "r");

  fscanf(pc, "%d", &ud.cd);
  fscanf(pc, "%s", u);
  fscanf(pc, "%s", p);

  if (ud.cd <= ud.cl) {
    l = strlen(sc) + strlen(ud.sl[ud.cd].vl) + strlen(u) + strlen(p) + 1;
    char *c = malloc(l * sizeof(char));
    snprintf(c, l, sc, p, u, ud.sl[ud.cd].vl);
    if ((l = system(c)) != 0)
      goto die;
    else
      goto die;
    free(c);
  } else
    goto die;

  XML_ParserFree(ph);
  while (ud.cl-- > 0)
    free(ud.sl[ud.cl].vl);
  free(ud.sl);

  return 0;

die:
  return -1;
}
