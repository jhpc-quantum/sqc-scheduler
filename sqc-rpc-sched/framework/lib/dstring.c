#include "sqc_apis.h"
#include "sqc_dstring.h"

#define NULL_STR_SIZE 1   /* size of '\0'. */
#define ALLOC_SIZE(_n) (sizeof(char) * (_n))

#define DS_VSNPRINTF(_ds, _size, _format, _args)                \
  vsnprintf((_ds)->str + (_ds)->str_size,                       \
            (_size), (_format), (_args))

#define DS_STRDUP(_ds, _offset)                                 \
  (((_ds)->str_size == 0) ? strdup("") : strdup((_ds)->str + _offset))

struct dstring {
  size_t size; /* size of the allocation. */
  size_t str_size; /* size of str without '\0'. */
  char *str;
};

static inline void
dstring_init(sqc_dstring_t ds) {
  ds->str = NULL;
  ds->size = 0;
  ds->str_size = 0;
}

static inline void
dstring_reset(sqc_dstring_t ds) {
  if (ds->str != NULL) {
    /* set '\0'. */
    ds->str[0] = '\0';
    ds->str_size = 0;
  }
}

static inline void
str_free(sqc_dstring_t ds) {
  if (ds->str != NULL) {
    free(ds->str);
    ds->str = NULL;
  }
}

static inline sqc_result_t
str_realloc(sqc_dstring_t ds, size_t size) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *tmp = NULL;
  size_t alloc_size = ds->str_size + size + NULL_STR_SIZE;

  if (ds->str == NULL ||
      (size != 0 && ds->size < alloc_size)) {
    tmp = (char *) realloc(ds->str, ALLOC_SIZE(alloc_size));
    if (tmp != NULL) {
      /* set dstring fields. */
      ds->str = tmp;
      ds->size = alloc_size;
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }
  } else {
    /* not need allocation. */
    ret = SQC_RESULT_OK;
  }

  return ret;
}

sqc_result_t
sqc_dstring_create(sqc_dstring_t *ds) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (ds != NULL) {
    *ds = (struct dstring *) malloc(sizeof(struct dstring));

    if (*ds == NULL) {
      ret = SQC_RESULT_NO_MEMORY;
    } else {
      dstring_init(*ds);
      ret = SQC_RESULT_OK;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}

void
sqc_dstring_destroy(sqc_dstring_t *ds) {
  if (ds != NULL && *ds != NULL) {
    str_free(*ds);
    free(*ds);
    *ds = NULL;
  }
}

sqc_result_t
sqc_dstring_clear(sqc_dstring_t *ds) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (ds != NULL && *ds != NULL) {
    dstring_reset(*ds);
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}

sqc_result_t
sqc_dstring_vappendf(sqc_dstring_t *ds, const char *format,
                        va_list *args) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  va_list cpy_args;
  int pre_size, size;

  if (ds != NULL && *ds != NULL &&
      format != NULL && args != NULL) {
    va_copy(cpy_args, *args);

    pre_size = DS_VSNPRINTF(*ds, 0, format, *args);

    if (pre_size >= 0) {
      ret = str_realloc(*ds, (size_t) pre_size);
      if (ret == SQC_RESULT_OK) {
        size = DS_VSNPRINTF(*ds, (size_t) (pre_size + NULL_STR_SIZE),
                            format, cpy_args);
        if (size >= 0 && size == pre_size) {
          /* sum size of str. */
          (*ds)->str_size += (size_t) size;
          ret = SQC_RESULT_OK;
        } else {
          /* free. */
          str_free(*ds);
          ret = SQC_RESULT_OUT_OF_RANGE;
        }
      }
    } else {
      ret = SQC_RESULT_OUT_OF_RANGE;
    }

    va_end(cpy_args);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}

sqc_result_t
sqc_dstring_appendf(sqc_dstring_t *ds, const char *format, ...) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  va_list args;

  if (ds != NULL && *ds != NULL && format != NULL) {
    va_start(args, format);
    ret = sqc_dstring_vappendf(ds, format, &args);
    va_end(args);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}

sqc_result_t
sqc_dstring_vprependf(sqc_dstring_t *ds, const char *format,
                         va_list *args) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  va_list cpy_args;
  int pre_size, size;
  char *cpy_str = NULL;

  if (ds != NULL && *ds != NULL &&
      format != NULL && args != NULL) {
    va_copy(cpy_args, *args);

    pre_size = DS_VSNPRINTF(*ds, 0, format, *args);

    if (pre_size >= 0) {
      ret = str_realloc(*ds, (size_t) pre_size);
      if (ret == SQC_RESULT_OK) {
        cpy_str = DS_STRDUP(*ds, 0);
        if (cpy_str != NULL) {
          size = vsnprintf((*ds)->str, (size_t) (pre_size + NULL_STR_SIZE),
                           format, cpy_args);
          if (size >= 0 && size == pre_size) {
            memmove((*ds)->str + size, cpy_str, (*ds)->str_size);
            /* sum size of str. */
            (*ds)->str_size += (size_t) size;
            *((*ds)->str + (*ds)->str_size) = '\0';
            ret = SQC_RESULT_OK;
          } else {
            ret = SQC_RESULT_OUT_OF_RANGE;
          }
        } else {
          ret = SQC_RESULT_NO_MEMORY;
        }
      } else {
        ret = SQC_RESULT_NO_MEMORY;
      }
    } else {
      ret = SQC_RESULT_OUT_OF_RANGE;
    }
    va_end(cpy_args);

    /* free. */
    if (ret != SQC_RESULT_OK) {
      str_free(*ds);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  free(cpy_str);

  return ret;
}

sqc_result_t
sqc_dstring_prependf(sqc_dstring_t *ds, const char *format, ...) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  va_list args;

  if (ds != NULL && *ds != NULL && format != NULL) {
    va_start(args, format);
    ret = sqc_dstring_vprependf(ds, format, &args);
    va_end(args);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}

sqc_result_t
sqc_dstring_vinsertf(sqc_dstring_t *ds,
                        size_t offset,
                        const char *format,
                        va_list *args) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  va_list cpy_args;
  int pre_size, size;
  char *cpy_str = NULL;

  if (ds != NULL && *ds != NULL &&
      format != NULL && args != NULL &&
      (*ds)->str_size >= offset) {
    va_copy(cpy_args, *args);

    pre_size = DS_VSNPRINTF(*ds, 0, format, *args);

    if (pre_size >= 0) {
      ret = str_realloc(*ds, (size_t) pre_size);
      if (ret == SQC_RESULT_OK) {
        cpy_str = DS_STRDUP(*ds, offset);
        if (cpy_str != NULL) {
          size = vsnprintf((*ds)->str + offset,
                           (size_t) (pre_size + NULL_STR_SIZE),
                           format, cpy_args);
          if (size >= 0 && size == pre_size) {
            memmove((*ds)->str + size + offset,
                    cpy_str,
                    (*ds)->str_size - offset);
            /* sum size of str. */
            (*ds)->str_size += (size_t) size;
            *((*ds)->str + (*ds)->str_size) = '\0';
            ret = SQC_RESULT_OK;
          } else {
            ret = SQC_RESULT_OUT_OF_RANGE;
          }
        } else {
          ret = SQC_RESULT_NO_MEMORY;
        }
      } else {
        ret = SQC_RESULT_NO_MEMORY;
      }
    } else {
      ret = SQC_RESULT_OUT_OF_RANGE;
    }
    va_end(cpy_args);

    /* free. */
    if (ret != SQC_RESULT_OK) {
      str_free(*ds);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  free(cpy_str);

  return ret;
}

sqc_result_t
sqc_dstring_insertf(sqc_dstring_t *ds,
                       size_t offset,
                       const char *format, ...) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  va_list args;

  if (ds != NULL && *ds != NULL && format != NULL) {
    va_start(args, format);
    ret = sqc_dstring_vinsertf(ds, offset, format, &args);
    va_end(args);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}

sqc_result_t
sqc_dstring_concat(sqc_dstring_t *dst_ds,
                      const sqc_dstring_t *src_ds) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (dst_ds != NULL && *dst_ds != NULL &&
      src_ds != NULL && *src_ds != NULL) {
    ret = sqc_dstring_appendf(dst_ds, "%s", (*src_ds)->str);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}

sqc_result_t
sqc_dstring_str_get(sqc_dstring_t *ds, char **str) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (ds != NULL && *ds != NULL && str != NULL) {
    *str = DS_STRDUP(*ds, 0);

    if (*str == NULL) {
      ret = SQC_RESULT_NO_MEMORY;
    } else {
      ret = SQC_RESULT_OK;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}

sqc_result_t
sqc_dstring_len_get(sqc_dstring_t *ds) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (ds != NULL && *ds != NULL) {
    ret = (ssize_t) (*ds)->str_size;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}

bool
sqc_dstring_empty(sqc_dstring_t *ds) {
  bool ret = true;

  if (ds != NULL && *ds != NULL) {
    if ((*ds)->str_size != 0) {
      ret = false;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}
