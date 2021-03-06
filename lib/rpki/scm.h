#ifndef _SCM_H_
#define _SCM_H_

/*
 * some constants for sizes of db tables
 */
#define DNAMESIZE 4096
#define FNAMESIZE 256
#define SUBJSIZE  512
#define SKISIZE   128
#define SIASIZE   1024
#define SIGSIZE   520
#define HASHSIZE  256

#define MANFILES_SIZE 400000

/*
 * A database table has four characteristics: its real name (the name by which
 * the database knows it), its user-friendly name, the SQL statement that
 * queries it, and the list of column names. The following data structure
 * captures that.
 */

typedef struct _scmtab {
    char *tabname;              /* SQL name of table */
    char *hname;                /* human readable name of table */
    char *tstr;                 /* table creation string */
    char **cols;                /* array of column names */
    int ncols;                  /* number of columns in "cols" */
} scmtab;

/*
 * This structure defines the overall database schema
 */

typedef struct _scm {
    char *db;                   /* name of the database */
    char *dbuser;               /* name of the database user */
    char *dbpass;               /* password for the database user */
    char *dsnpref;              /* data source name prefix */
    char *dsn;                  /* canonical data source name */
    scmtab *tables;             /* array of tables */
    int ntables;                /* number of tables in "tables" */
} scm;

extern scm *initscm(
    void);
extern void freescm(
    scm *scmp);
extern char *makedsnscm(
    const char *pref,
    const char *db,
    const char *usr,
    const char *pass);
extern scmtab *findtablescm(
    scm *scmp,
    const char *hname);

#endif
