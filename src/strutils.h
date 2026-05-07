#ifndef STRUTILS_H
#define STRUTILS_H 1

#ifdef __cplusplus
extern "C" {
#endif

/*@ @brief For a linked list of strings, as filled by strparse() */
typedef struct SLstrlist_s {
  char               *element;
  struct SLstrlist_s *next;
} SLstrlist;

extern int sl_strparse (const char *string, const char *delim, SLstrlist **list);

#ifdef __cplusplus
}
#endif

#endif /* STRUTILS_H */

