#include "sqc_apis.h"





#include "hash.h"
#include "hash.c"





typedef struct sqc_hashmap_record {
  sqc_hashmap_type_t m_type;
  sqc_rwlock_t m_lock;
  HashTable m_hashtable;
  sqc_hashmap_value_freeup_proc_t m_del_proc;
  ssize_t m_n_entries;
  bool m_is_operational;
} sqc_hashmap_record;





static inline void
s_read_lock(sqc_hashmap_t hm, int *ostateptr) {
  if (hm != NULL && ostateptr != NULL) {
    (void)sqc_rwlock_reader_enter_critical(&(hm->m_lock), ostateptr);
  }
}


static inline void
s_write_lock(sqc_hashmap_t hm, int *ostateptr) {
  if (hm != NULL && ostateptr != NULL) {
    (void)sqc_rwlock_writer_enter_critical(&(hm->m_lock), ostateptr);
  }
}


static inline void
s_unlock(sqc_hashmap_t hm, int ostate) {
  if (hm != NULL) {
    (void)sqc_rwlock_leave_critical(&(hm->m_lock), ostate);
  }
}


static inline bool
s_do_iterate(sqc_hashmap_t hm,
             sqc_hashmap_iteration_proc_t proc, void *arg) {
  bool ret = false;
  if (hm != NULL && proc != NULL) {
    HashSearch s;
    sqc_hashentry_t he;

    for (he = FirstHashEntry(&(hm->m_hashtable), &s);
         he != NULL;
         he = NextHashEntry(&s)) {
      if ((ret = proc(GetHashKey(&(hm->m_hashtable), he),
                      GetHashValue(he),
                      he,
                      arg)) == false) {
        break;
      }
    }
  }
  return ret;
}


static inline sqc_hashentry_t
s_find_entry(sqc_hashmap_t hm, const void *key) {
  sqc_hashentry_t ret = NULL;

  if (hm != NULL) {
    ret = FindHashEntry(&(hm->m_hashtable), key);
  }

  return ret;
}


static inline sqc_hashentry_t
s_create_entry(sqc_hashmap_t hm, const void *key) {
  sqc_hashentry_t ret = NULL;

  if (hm != NULL) {
    int is_new;
    ret = CreateHashEntry(&(hm->m_hashtable), key, &is_new);
  }

  return ret;
}


static bool
s_freeup_proc(const void *key, void *val, sqc_hashentry_t he, void *arg) {
  bool ret = false;
  (void)key;
  (void)he;

  if (arg != NULL) {
    sqc_hashmap_t hm = (sqc_hashmap_t)arg;
    if (hm->m_del_proc != NULL) {
      if (val != NULL) {
        hm->m_del_proc(val);
      }
      ret = true;
    }
  }

  return ret;
}


static inline void
s_freeup_all_values(sqc_hashmap_t hm) {
  s_do_iterate(hm, s_freeup_proc, (void *)hm);
}


static inline void
s_clean(sqc_hashmap_t hm, bool free_values) {
  if (free_values == true) {
    s_freeup_all_values(hm);
  }
  DeleteHashTable(&(hm->m_hashtable));
  (void)memset(&(hm->m_hashtable), 0, sizeof(HashTable));
  hm->m_n_entries = 0;
}


static inline void
s_reinit(sqc_hashmap_t hm, bool free_values) {
  s_clean(hm, free_values);
  InitHashTable(&(hm->m_hashtable), (unsigned int)hm->m_type);
}





void
sqc_hashmap_set_value(sqc_hashentry_t he, void *val) {
  if (he != NULL) {
    SetHashValue(he, val);
  }
}


sqc_result_t
sqc_hashmap_create(sqc_hashmap_t *retptr,
                      sqc_hashmap_type_t t,
                      sqc_hashmap_value_freeup_proc_t proc) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_hashmap_t hm;

  if (retptr != NULL) {
    *retptr = NULL;
    hm = (sqc_hashmap_t)malloc(sizeof(*hm));
    if (hm != NULL) {
      if ((ret = sqc_rwlock_create(&(hm->m_lock))) ==
          SQC_RESULT_OK) {
        hm->m_type = t;
        InitHashTable(&(hm->m_hashtable), (unsigned int)t);
        hm->m_del_proc = proc;
        hm->m_n_entries = 0;
        hm->m_is_operational = true;
        *retptr = hm;
        ret = SQC_RESULT_OK;
      } else {
        free((void *)hm);
      }
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
sqc_hashmap_shutdown(sqc_hashmap_t *hmptr, bool free_values) {
  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      if ((*hmptr)->m_is_operational == true) {
        (*hmptr)->m_is_operational = false;
        s_clean(*hmptr, free_values);
      }
    }
    s_unlock(*hmptr, cstate);

  }
}


void
sqc_hashmap_destroy(sqc_hashmap_t *hmptr, bool free_values) {
  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      if ((*hmptr)->m_is_operational == true) {
        (*hmptr)->m_is_operational = false;
        s_clean(*hmptr, free_values);
      }
    }
    s_unlock(*hmptr, cstate);

    sqc_rwlock_destroy(&((*hmptr)->m_lock));
    free((void *)*hmptr);
    *hmptr = NULL;
  }
}





static inline sqc_result_t
s_clear(sqc_hashmap_t *hmptr, bool free_values) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if ((*hmptr)->m_is_operational == true) {
    (*hmptr)->m_is_operational = false;
    s_reinit(*hmptr, free_values);
    (*hmptr)->m_is_operational = true;
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


sqc_result_t
sqc_hashmap_clear(sqc_hashmap_t *hmptr, bool free_values) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      ret = s_clear(hmptr, free_values);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_hashmap_clear_no_lock(sqc_hashmap_t *hmptr, bool free_values) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {
    ret = s_clear(hmptr, free_values);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline sqc_result_t
s_find(sqc_hashmap_t *hmptr,
       const void *key, void **valptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_hashentry_t he;

  *valptr = NULL;

  if ((*hmptr)->m_is_operational == true) {
    if ((he = s_find_entry(*hmptr, key)) != NULL) {
      *valptr = GetHashValue(he);
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NOT_FOUND;
    }
  } else {
    ret = SQC_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


sqc_result_t
sqc_hashmap_find_no_lock(sqc_hashmap_t *hmptr,
                            const void *key, void **valptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {

    ret = s_find(hmptr, key, valptr);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_hashmap_find(sqc_hashmap_t *hmptr, const void *key, void **valptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {
    int cstate;

    s_read_lock(*hmptr, &cstate);
    {
      ret = s_find(hmptr, key, valptr);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline sqc_result_t
s_add(sqc_hashmap_t *hmptr,
      const void *key, void **valptr,
      bool allow_overwrite) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  void *oldval = NULL;
  sqc_hashentry_t he;

  if ((*hmptr)->m_is_operational == true) {
    if ((he = s_find_entry(*hmptr, key)) != NULL) {
      oldval = GetHashValue(he);
      if (allow_overwrite == true) {
        SetHashValue(he, *valptr);
        ret = SQC_RESULT_OK;
      } else {
        ret = SQC_RESULT_ALREADY_EXISTS;
      }
    } else {
      he = s_create_entry(*hmptr, key);
      if (he != NULL) {
        SetHashValue(he, *valptr);
        (*hmptr)->m_n_entries++;
        ret = SQC_RESULT_OK;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
      }
    }
    *valptr = oldval;
  } else {
    ret = SQC_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


sqc_result_t
sqc_hashmap_add(sqc_hashmap_t *hmptr,
                   const void *key, void **valptr,
                   bool allow_overwrite) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      ret = s_add(hmptr, key, valptr, allow_overwrite);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_hashmap_add_no_lock(sqc_hashmap_t *hmptr,
                           const void *key, void **valptr,
                           bool allow_overwrite) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      valptr != NULL) {

    ret = s_add(hmptr, key, valptr, allow_overwrite);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline sqc_result_t
s_delete(sqc_hashmap_t *hmptr,
         const void *key, void **valptr,
         bool free_value) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  void *val = NULL;
  sqc_hashentry_t he;

  if ((*hmptr)->m_is_operational == true) {
    if ((he = s_find_entry(*hmptr, key)) != NULL) {
      val = GetHashValue(he);
      if (val != NULL &&
          free_value == true &&
          (*hmptr)->m_del_proc != NULL) {
        (*hmptr)->m_del_proc(val);
      }
      DeleteHashEntry(he);
      (*hmptr)->m_n_entries--;
    }
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_NOT_OPERATIONAL;
  }

  if (valptr != NULL) {
    *valptr = val;
  }

  return ret;
}


sqc_result_t
sqc_hashmap_delete(sqc_hashmap_t *hmptr,
                      const void *key, void **valptr,
                      bool free_value) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_write_lock(*hmptr, &cstate);
    {
      ret = s_delete(hmptr, key, valptr, free_value);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_hashmap_delete_no_lock(sqc_hashmap_t *hmptr,
                              const void *key, void **valptr,
                              bool free_value) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {

    ret = s_delete(hmptr, key, valptr, free_value);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline sqc_result_t
s_iterate(sqc_hashmap_t *hmptr,
          sqc_hashmap_iteration_proc_t proc,
          void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if ((*hmptr)->m_is_operational == true) {
    if (s_do_iterate(*hmptr, proc, arg) == true) {
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_ITERATION_HALTED;
    }
  } else {
    ret = SQC_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


sqc_result_t
sqc_hashmap_iterate(sqc_hashmap_t *hmptr,
                       sqc_hashmap_iteration_proc_t proc,
                       void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      proc != NULL) {
    int cstate;

    /*
     * The proc could modify hash values so we use write lock.
     */
    s_write_lock(*hmptr, &cstate);
    {
      ret = s_iterate(hmptr, proc, arg);
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_hashmap_iterate_no_lock(sqc_hashmap_t *hmptr,
                               sqc_hashmap_iteration_proc_t proc,
                               void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      proc != NULL) {

    ret = s_iterate(hmptr, proc, arg);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





sqc_result_t
sqc_hashmap_size(sqc_hashmap_t *hmptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {
    int cstate;

    s_read_lock(*hmptr, &cstate);
    {
      if ((*hmptr)->m_is_operational == true) {
        ret = (*hmptr)->m_n_entries;
      } else {
        ret = SQC_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_hashmap_size_no_lock(sqc_hashmap_t *hmptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL) {

    if ((*hmptr)->m_is_operational == true) {
      ret = (*hmptr)->m_n_entries;
    } else {
      ret = SQC_RESULT_NOT_OPERATIONAL;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_hashmap_statistics(sqc_hashmap_t *hmptr, const char **msgptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (hmptr != NULL &&
      *hmptr != NULL &&
      msgptr != NULL) {
    int cstate;

    *msgptr = NULL;

    s_read_lock(*hmptr, &cstate);
    {
      if ((*hmptr)->m_is_operational == true) {
        *msgptr = (const char *)HashStats(&((*hmptr)->m_hashtable));
        if (*msgptr != NULL) {
          ret = SQC_RESULT_OK;
        } else {
          ret = SQC_RESULT_NO_MEMORY;
        }
      } else {
        ret = SQC_RESULT_NOT_OPERATIONAL;
      }
    }
    s_unlock(*hmptr, cstate);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
sqc_hashmap_atfork_child(sqc_hashmap_t *hmptr) {
  if (hmptr != NULL &&
      *hmptr != NULL) {
    sqc_rwlock_reinitialize(&((*hmptr)->m_lock));
  }
}
