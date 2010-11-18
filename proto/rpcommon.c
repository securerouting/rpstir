/*
  $Id: rpwork.h 888 2009-11-17 17:59:35Z gardiner $
*/

/* ***** BEGIN LICENSE BLOCK *****
 *
 * BBN Address and AS Number PKI Database/repository software
 * Version 3.0-beta
 *
 * US government users are permitted unrestricted rights as
 * defined in the FAR.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT
 * WARRANTY OF ANY KIND, either express or implied.
 *
 * Copyright (C) Raytheon BBN Technologies Corp. 2009-2010.  All Rights Reserved.
 *
 * Contributor(s):  Charles Gardiner
 *
 * ***** END LICENSE BLOCK ***** */
#include "rpwork.h"
#include <time.h>
#include <fcntl.h>


struct done_certs done_certs;

extern char nextskibuf[SKIBUFSIZ];
extern struct Certificate myrootcert;
extern char myrootfullname[PATH_MAX];
extern char errbuf[160];
extern struct ipranges certranges, ruleranges, lessranges, fromranges;

extern char *Xcrldp, *Xcp, *Xrpdir;


struct keyring keyring;

void free_keyring()
  {
  if (keyring.filename) free(keyring.filename);
  if (keyring.label) free(keyring.label);
  if (keyring.password) free(keyring.password);
  keyring.filename = keyring.label = keyring.password = (char *)0;
  }

static int check_keyring(char *cc)
  {
  char *b;
  if ((cc = nextword(cc)))
    {
    b = strchr(cc, (int)' ');
    if (b)
      {
      keyring.filename = (char *)calloc(1, (b - cc) + 2);
      strncpy(keyring.filename, cc, (b - cc));
      if ((cc = nextword(cc)))
        {
        if ((b = strchr(cc, (int)' ')))
          {
          keyring.label = (char *)calloc(1, (b - cc) + 2);
          strncpy( keyring.label, cc, (b - cc));
          if ((cc = nextword(cc)))
            {
            for (b = cc; *b > ' '; b++);
            keyring.password = (char *)calloc(1, (b - cc) + 2);
            strncpy( keyring.password, cc, (b - cc));
            return 1;
            }
          }
        }
      }
    }
  return -1;
  }

char *nextword(char *cc)
  {
  char *b = strchr(cc, (int)' ');
  if (b) while (*b && *b == ' ') b++;
  return b;
  }

static int trueOrFalse(char *c)
  {
  if (!strcmp(c, "TRUE") || !strcmp(c, "FALSE")) return 1;
  return 0;
  }

static char *next_cmd(char *buf, int siz, FILE *SKI)
  {
  char *c;
  do
    {
    if (!(c = fgets(buf, siz, SKI))) return c;
    }
  while(*buf == ';');
  char *cc;
  for (cc = buf; *cc != '\n'; cc++)
      {
      if (*cc == '\t' || *cc == ' ')
        {
        *cc = ' ';
        while (cc[1] == ' ' || cc[1] == '\t') strcpy(&cc[1], &cc[2]);
        }
      }
  return c;
  }

static int check_cp(char *cpp)
  {
  if ((*cpp == 'C' || *cpp == 'D' || *cpp == 'R') && cpp[1] <= ' ')
    {

    Xcp = (char *)calloc(1, 2);
    *Xcp = *cpp;
    }
  else
    {
    char *c;
    for (c = cpp; *c > ' ' && (*c == '.' || (*c >= '0' && *c <= '9')); c++);
    if (*c == ' ') *c = 0;
    if (*c > ' ') return -1;
    struct casn oid;
    tagged_constructor(&oid, 0, ASN_OBJ_ID, ASN_OBJ_ID);
    int ansr = write_objid(&oid, cpp);
    clear_casn(&oid);
    if (ansr > 0)
      {
      Xcp = (char *)calloc(1, strlen(cpp) + 2);
      strcpy(Xcp, cpp);
      }
    else return -1;
    }
  return 1;
  }

int check_date(char *datep, struct casn *casnp, ulong *datenump)
  {
  char *c;
  for (c = datep; *c >= '0' && *c <= '9'; c++);
  if (*c != 'Z' || c != &datep[14]) return -1;
  if (strncmp(datep, "2000", 4) < 0) return -1;
  ulong tag;
  if (strncmp(datep, "2050", 4) >= 0) tag = (ulong)ASN_GENTIME;
  else tag = (ulong)ASN_UTCTIME;
  tagged_constructor(casnp, 0, tag, tag);
  if ((tag == (ulong)ASN_UTCTIME &&
    (write_casn(casnp, (uchar *)&datep[2], 13) < 0 ||
    read_casn_time(casnp, datenump)) < 0) ||
    (tag == (ulong)ASN_GENTIME &&
    (write_casn(casnp, (uchar *)datep, 15) < 0 ||
    read_casn_time(casnp, datenump) < 0))) return -1;
  return 1;
  }

int check_dates(char *datesp)
  {
  ulong fromDate, toDate;
  time_t now = time((time_t *)0);
  char *enddatep = nextword(datesp);
  if (!enddatep || datesp[14] != 'Z' || datesp[15] != ' ' ||
    enddatep[14] != 'Z' || enddatep[15] > ' ' ||
    strncmp(datesp, enddatep, 14) >= 0) return -1;
    if (check_date(datesp, &Xvaliddates.lodate, &fromDate) < 0 ||
      check_date(enddatep, &Xvaliddates.hidate, &toDate) < 0 ||
      fromDate >= toDate || toDate < now) return -1;
  return 1;
  }

struct Extension *find_extn(struct Certificate *certp, char *oid,
  int add)
  {
  struct Extensions *extsp = &certp->toBeSigned.extensions;
  struct Extension *extp;
  int num = num_items(&extsp->self);
  if (!num && !add) return (struct Extension *)0;
  for (extp = (struct Extension *)member_casn(&extsp->self, 0);
    extp && diff_objid(&extp->extnID, oid);
    extp = (struct Extension *)next_of(&extp->self));
  if (!extp && add)
    {
    extp = (struct Extension *)inject_casn(&extsp->self, num);
    }
  return extp;
  }

void free_ipranges(struct ipranges *iprangesp)
  {
  int i;
  struct iprange *iprangep = iprangesp->iprangep;
  if (!iprangep) return;
  for (i = 0; i < iprangesp->numranges; i++, iprangep++)
    {
    if (iprangep->text) 
      {
      free(iprangep->text);
      iprangep->text = (char *)0;
      } 
    }
  free(iprangesp->iprangep);
  iprangesp->iprangep = (struct iprange *)0;
  }

void clear_ipranges(struct ipranges *iprangesp)
  {
  free_ipranges(iprangesp);
  iprangesp->iprangep = (struct iprange *)0;
  iprangesp->numranges = 0;
  }

static void internal_error(char *msg)
  {
  fprintf(stderr, msg);
  exit(0);
  }

struct iprange *eject_range(struct ipranges *iprangesp, int num)
  {
  int typ;
  if (num < 0 || num >= iprangesp->numranges)
    internal_error("Ejecting out of range\n");
  iprangesp->numranges--;
  struct iprange *newrangep = (struct iprange *)calloc(
    iprangesp->numranges, sizeof(struct iprange));
  int i;
  for(i = 0; i < num; i++)
    {
    newrangep[i].typ = iprangesp->iprangep[i].typ;
    memcpy(newrangep[i].lolim, iprangesp->iprangep[i].lolim, 18);
    memcpy(newrangep[i].hilim, iprangesp->iprangep[i].hilim, 18);
    newrangep[i].text = iprangesp->iprangep[i].text;
    iprangesp->iprangep[i].text = (char *)0;
    }
  typ = iprangesp->iprangep[i].typ;
  for( ; i < iprangesp->numranges; i++)
    {
    newrangep[i].typ = iprangesp->iprangep[i + 1].typ;
    memcpy(newrangep[i].lolim, iprangesp->iprangep[i + 1].lolim, 18);
    memcpy(newrangep[i].hilim, iprangesp->iprangep[i + 1].hilim, 18);
    newrangep[i].text = iprangesp->iprangep[i + 1].text;
    iprangesp->iprangep[i].text = (char *)0;
    }
  free_ipranges(iprangesp);
  if (iprangesp->numranges)
    {
    iprangesp->iprangep = newrangep;
    if (iprangesp->iprangep[num].typ != typ) return NULL;
    }
  return &iprangesp->iprangep[num];
  }

struct iprange *inject_range(struct ipranges *iprangesp, int num)
  {
  if (num < 0 || num > iprangesp->numranges)
    internal_error("Injecting out of range\n");
  struct iprange *newrangep = (struct iprange *)calloc(
    iprangesp->numranges + 1, sizeof(struct iprange));
  int i;
  for(i = 0; i < num; i++)
    {
    newrangep[i].typ = iprangesp->iprangep[i].typ;
    memcpy(newrangep[i].lolim, iprangesp->iprangep[i].lolim, 18);
    memcpy(newrangep[i].hilim, iprangesp->iprangep[i].hilim, 18);
    newrangep[i].text = iprangesp->iprangep[i].text;
    iprangesp->iprangep[i].text = (char *)0;
    }
  memset(newrangep[i].lolim, 0, sizeof(newrangep));
  memset(newrangep[i].hilim, 0, sizeof(newrangep));
  newrangep[i].text = 0;
  for( ; i < iprangesp->numranges; i++)
    {
    newrangep[i + 1].typ = iprangesp->iprangep[i].typ;
    memcpy(newrangep[i + 1].lolim, iprangesp->iprangep[i].lolim, 18);
    memcpy(newrangep[i + 1].hilim, iprangesp->iprangep[i].hilim, 18);
    newrangep[i + 1].text = iprangesp->iprangep[i].text;
    iprangesp->iprangep[i].text = (char *)0;
    }
  free_ipranges(iprangesp);
  iprangesp->numranges++;
  iprangesp->iprangep = newrangep;
  return &iprangesp->iprangep[num];
  }

struct iprange *next_range(struct ipranges *iprangesp,
  struct iprange *iprangep)
  {
  if (iprangep[1].typ != iprangep->typ) return (struct iprange *)0;
  return ++iprangep;
  }

int sort_resources(struct iprange *iprangesp, int numranges)
  {
  struct iprange *rp0, *rp1, spare;
  int did, i;
  for (did = 0, i = 1; i < numranges; )
    {
    rp0 = &iprangesp[i - 1];
    rp1 = &iprangesp[i];
    if (diff_ipaddr(rp0, rp1) > 0) // swap them
      {
      memcpy(&spare, rp0, sizeof(struct iprange));
      memcpy(rp0, rp1,    sizeof(struct iprange));
      memcpy(rp1, &spare, sizeof(struct iprange));
      i = 1;    // go back to start
      did++;
      }
    else i++;
    }
  return did;
  } 

int touches(struct iprange *lop, struct iprange *hip, int lth)
  {
  struct iprange mid;
  memcpy(mid.lolim, lop->hilim, lth);
  increment_iprange(mid.lolim, lth);
  return memcmp(mid.lolim, hip->lolim, lth);
  }

static int getIPBlock(FILE *SKI, int typ, char *skibuf, int siz)
  {
  char *c;
  while((c = next_cmd(skibuf, siz, SKI)))
    {
    if (*skibuf <= ' ') continue;
    if ((typ == IPv4 && *skibuf == 'I') ||  
      (typ == IPv6 && !strncmp(skibuf, "AS", 2)) ||
      (typ == ASNUM && *skibuf == 'S')) break;
    char *cc = nextword(skibuf);
    if  (cc && *cc != '-') return ERR_SCM_BADSKIBLOCK;
    struct iprange *iprangep  = inject_range(&ruleranges, ruleranges.numranges);
    if (txt2loc(typ, skibuf, iprangep) < 0) return ERR_SCM_BADRANGE;
    else
      {
      int j = strlen(skibuf);
      iprangep->text = calloc(1, j);
      strncpy(iprangep->text, skibuf, j - 1);
      if (iprangep > &ruleranges.iprangep[0] &&
        iprangep->typ == iprangep[-1].typ &&
        (j = touches(&iprangep[-1], iprangep, (iprangep->typ == IPv4)? 4: 16))
         >= 0)
        {
        strcpy(errbuf, (!j)? "Ranges touch ": "Ranges out of order ");
        return  ERR_SCM_BADSKIBLOCK;
        }
      }
    }
  if (!c) *skibuf = 0;
  if (typ == ASNUM && c && !strncmp(skibuf, "SKI", 3))
    strcpy(nextskibuf, skibuf);
  return (c)? 1: 0;
  }

int getSKIBlock(FILE *SKI, char *skibuf, int siz)
  {
  int ansr = ERR_SCM_BADSKIBLOCK;
  if (!next_cmd(skibuf, siz, SKI) || strcmp(skibuf, "IPv4\n"))
    strcpy(errbuf, "Missing/invalid IPv4 ");
  else if (getIPBlock(SKI, IPv4, skibuf, siz) < 0)
    {
    if (!*errbuf) strcpy(errbuf, "Bad/disordered IPv4 group ");
    }
  else if (strcmp(skibuf, "IPv6\n")) 
    strcpy(errbuf, "Missing/invalid IPv6 ");
  else if (getIPBlock(SKI, IPv6, skibuf, siz) < 0)
    strcpy(errbuf, "Bad/disordered IPv6 group ");
  else if (strcmp(skibuf, "AS#\n"))
    strcpy(errbuf, "Missing/invalid AS# ");
  else if(getIPBlock(SKI, ASNUM, skibuf, siz) < 0)
    strcpy(errbuf, "Bad/disordered AS# group ");
  else if (ruleranges.numranges == 0)
    strcpy(errbuf, "Empty SKI block ");
  else 
    {
    ansr = 1;
    }  
  return ansr;
  }

int parse_SKI_blocks(FILE *SKI, FILE *logfile, char *skibuf, int siz, 
  int *locflagsp)
  {
/*
Procedure:
1. Open file for control data
   Get certificate for RP
   Get key information for RP
   Get any flags
   Get first SKI line from the control file
2. Process all the control blocks
   IF no error,
     FOR each item in done_certs
       Flag the target cert in the database as having a para
       Sign the paracertificate
       Put it into database with para flag
   Free all and return error
*/
  Certificate(&myrootcert, (ushort)0);
  char *c, *cc;
                                                     // step 1
  int ansr = 0;

  if (!next_cmd(skibuf, siz, SKI) ||
    strncmp(skibuf, "PRIVATEKEYMETHOD", 16))
    {
    ansr = ERR_SCM_BADSKIFILE;
    strcpy(errbuf, "No private key method.");
    }
  else
    {
    for (cc = &skibuf[16]; *cc && *cc <= ' '; cc++);
    if (strncmp(cc, "Keyring", 7) || check_keyring(cc) < 0)
      {
      ansr = ERR_SCM_BADSKIFILE;
      strcpy(errbuf, "Invalid private key method.");
      }
    }
  if (!ansr)
    {
    if (!next_cmd(skibuf, siz, SKI) ||
      strncmp(skibuf, "TOPLEVELCERTIFICATE ", 20))
      {
      ansr = ERR_SCM_NORPCERT;
      strcpy(errbuf, "No top level certificate.");
      }
    else
      {           // get root cert
      if ((c = strchr(skibuf, (int)'\n'))) *c = 0;
      strcpy(myrootfullname, &skibuf[20]);
      if (get_casn_file(&myrootcert.self, &skibuf[20], 0) < 0)
        {
        sprintf(errbuf, "Invalid top level certificate: %s.", myrootfullname);
        ansr = ERR_SCM_NORPCERT;
        }
      else
        {
        for (c--; c > &skibuf[20] && *c != '/'; c--);
        if (*c != '/')
          {
          char *rootp = getenv("RPKI_ROOT");
          Xrpdir = (char *)calloc(1, strlen(rootp) + strlen(c) + 4);
          sprintf(Xrpdir, "%s/%s", rootp, c);
          }
        else
          {
          *c = 0;
          Xrpdir = (char *)calloc(1, strlen(&skibuf[20]) + 4);
          strcpy(Xrpdir, &skibuf[20]);
          }
        }
      }
    }
  if (!ansr && !next_cmd(skibuf, siz, SKI))
    {
    strcpy(errbuf, "No control section.");
    ansr = ERR_SCM_BADSKIFILE;
    }
  else if (!ansr)   // CONTROL section
    {
    c = skibuf;
    while (c && !ansr && !strncmp(skibuf, "CONTROL ", 8))
      {
      if ((c = strchr(skibuf, (int)'\n'))) *c = 0;
      cc = nextword(skibuf);
      if (!strncmp(cc, "treegrowth", 10) && cc[10] == ' ')
        {
        cc = nextword(cc);
        if (!trueOrFalse(cc)) ansr = -1;
        else if (*cc == 'T') *locflagsp |= TREEGROWTH;
        }
      else if (!strncmp(cc, "resource_nounion", 16) && cc[16] == ' ')
        {
        cc = nextword(cc);
        if (!trueOrFalse(cc)) ansr = -1;
        else if (*cc == 'T') *locflagsp |= RESOURCE_NOUNION;
        }
      else if (!strncmp(cc, "intersection_always", 19) && cc[19] == ' ')
        {
        cc = nextword(cc);
        if (!trueOrFalse(cc)) ansr = -1;
        else if (*cc == 'T') *locflagsp |= INTERSECTION_ALWAYS;
        }
      else
        {
        ansr = ERR_SCM_BADSKIFILE;
        sprintf(errbuf, "Invalid control message: %s.\n", cc);
        }
      if (!ansr) c = next_cmd(skibuf, siz, SKI);
      }
    if (ansr == -1)
      {
      sprintf(errbuf, "No/not TRUE or FALSE in %s.", skibuf);
      ansr = ERR_SCM_BADSKIFILE;
      }
    while (c && !ansr && !strncmp(skibuf, "TAG", 3))
      {
      if ((c = strchr(skibuf, (int)'\n'))) *c = 0;
      cc = nextword(skibuf);
      if (skibuf[3] != ' ')
        {
        sprintf(errbuf, "Invalid line: %s.", skibuf);
        ansr = ERR_SCM_BADSKIFILE;
        break;
        }
      if (!strncmp(cc, "Xvalidity_dates ", 16))
        {
        cc = nextword(cc);
        if (!*cc || (*cc != 'C'  && *cc != 'R' && check_dates(cc) < 0))
          {
          ansr = ERR_SCM_BADSKIFILE;
          break;
          }
        }
      else if (!strncmp(cc, "Xcrldp ", 7))
        {
        cc = nextword(cc);
        if (!*cc || (*cc == 'R' && cc[1] <= ' ' &&
          !find_extn(&myrootcert, id_cRLDistributionPoints, 0)))
          ansr = ERR_SCM_BADSKIFILE;
        else if (strchr(cc, (int)','))
          {
          ansr = ERR_SCM_BADSKIFILE;
          break;
          }
        else
          { 
          Xcrldp = (char *)calloc(1, strlen(cc) + 2);
          strcpy(Xcrldp, cc);
          }
        }
      else if (!strncmp(cc, "Xcp ", 4))
        {
        struct Extension *extp;
        cc = nextword(cc);
        if (!*cc ||
          (*cc == 'R' &&
          ((!(extp = find_extn(&myrootcert, id_certificatePolicies, 0)))
          ||
          num_items(&extp->extnValue.certificatePolicies.self) > 1)))
          ansr = ERR_SCM_BADSKIFILE;
        else if (nextword(cc))
          {
          ansr = ERR_SCM_BADSKIFILE;
          sprintf(errbuf, "Invalid Xcp entry: %s.", skibuf);
          } 
        else if (check_cp(cc) < 0) ansr = ERR_SCM_BADSKIFILE;
        }
      else if (!strncmp(cc, "Xaia ", 5))
        {
        cc = nextword(cc);
        Xaia = (char *)calloc(1, strlen(cc));
        strncpy(Xaia, cc, strlen(cc));
        }
      else
        {
        ansr = ERR_SCM_BADSKIFILE;
        sprintf(errbuf, "Invalid TAG entry: %s.", cc);
        }
      if (!ansr) c = next_cmd(skibuf, siz, SKI);
      }
    if (!*errbuf && !strncmp(skibuf, "CONTROL ", 8))
      {
      sprintf(errbuf, "CONTROL message out of order: %s", skibuf);
      ansr = ERR_SCM_BADSKIFILE;
      }
    else if (ansr < 0)
      {
      if ((c = strchr(skibuf, (int)'\n'))) *c = 0;
      if (!*errbuf) sprintf(errbuf, "Invalid entry in file: %s.", skibuf);
      }
    else if (!ansr)
      {
      if (!c || strncmp(skibuf, "SKI ", 4))
        {
        ansr = ERR_SCM_BADSKIFILE;
        sprintf(errbuf, "No SKI entry in file.");
        }
      else if (!(cc = nextword(skibuf)) || *cc < ' ')
        {
        ansr = ERR_SCM_BADSKIFILE;
        strcpy(errbuf, "Incomplete SKI entry.");
        }
      }
    }
  return ansr;
  }

