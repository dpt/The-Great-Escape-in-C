#ifndef UTILS_H
#define UTILS_H

#define UNKNOWN 1

#define NELEMS(a) ((int) (sizeof(a) / sizeof(a[0])))

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/**
 * Given a pointer p, its parent structure type t and structure member name f,
 * returns the parent structure.
 */
#define structof(p,t,f) ((t*)((uintptr_t)(p) - offsetof(t,f)))

#endif /* UTILS_H */
