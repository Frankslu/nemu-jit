/*
* you need to implement a structure with NO and *next
* example:
*   typedef struct watchpoint {
*     int NO;
*     struct watchpoint *next;
*     char *str;
*   } WP;
*/
#define isstatic(x) IFDEF(x, static)

#if !defined(T) || !defined(pool_name) || !defined(NR_POOL) || \
  !defined(pool_head) || !defined(pool_free)
#error "you need to implement T(type of pool), pool_name, pool_head, \
  pool_free at first"
#endif

#if defined(T) || defined(pool_name) || defined(NR_POOL) || \
  defined(pool_head) || defined(pool_free)
static T pool_name[NR_POOL + 2] = {};
static T *pool_head = NULL, *pool_free = NULL;

#ifdef func_init_chain_name
void func_init_chain_name() {
  int i;
  for (i = 0; i < NR_POOL - 1; i ++) {
    pool_name[i].NO = i;
    pool_name[i].next = &pool_name[i + 1];
  }
  pool_name[i].NO = i;
  pool_name[i].next = NULL;
  pool_head = &pool_name[NR_POOL];
  pool_head->next = NULL;
  pool_free = &pool_name[NR_POOL + 1];
  pool_free->next = pool_name;
}
#endif

#ifdef func_insert_chain_name
isstatic(func_insert_chain_static)
void func_insert_chain_name(T *node, T *list){
  T *p = list;
  while (p->next != NULL && p->next->NO < node->NO) {
    p = p->next;
  }
  node->next = p->next;
  p->next = node;
}
#endif

#ifdef func_find_prev_chain_name
isstatic(func_find_prev_chain_static)
T *func_find_prev_chain_name(int NO, T *list) {
  T *p = list;
  while (p->next != NULL && p->next->NO < NO) {
    p = p->next;
  }
  if (p->next == NULL)
    return NULL;
  return p->next->NO == NO ? p : NULL;
}
#endif
#endif