#ifndef __SQC_STRUTILS_H__
#define __SQC_STRUTILS_H__





/**
 *	@file	sqc_strutils.h
 */





__BEGIN_DECLS





sqc_result_t
sqc_str_tokenize_with_limit(char *buf, char **tokens,
                               size_t max, size_t limit, const char *delm);

#define sqc_str_tokenize(_buf, _tokens, _max, _delm) \
  sqc_str_tokenize_with_limit((_buf), (_tokens), (_max), 0, (_delm))

sqc_result_t
sqc_str_tokenize_quote(char *buf, char **tokens,
                          size_t max, const char *delm, const char *quote);

sqc_result_t
sqc_str_unescape(const char *org, const char *escaped,
                    char **retptr);

sqc_result_t
sqc_str_escape(const char *in_str, const char *escape_chars,
                  bool *is_escaped, char **out_str);

sqc_result_t
sqc_str_trim_right(const char *org, const char *trim_chars,
                      char **retptr);

sqc_result_t
sqc_str_trim_left(const char *org, const char *trimchars,
                     char **retptr);

sqc_result_t
sqc_str_trim(const char *org, const char *trimchars,
                char **retptr);





sqc_result_t
sqc_str_parse_int16_by_base(const char *buf, int16_t *val,
                               unsigned int base);
sqc_result_t
sqc_str_parse_int16(const char *buf, int16_t *val);
sqc_result_t
sqc_str_parse_uint16_by_base(const char *buf, uint16_t *val,
                                unsigned int base);
sqc_result_t
sqc_str_parse_uint16(const char *buf, uint16_t *val);





sqc_result_t
sqc_str_parse_int32_by_base(const char *buf, int32_t *val,
                               unsigned int base);
sqc_result_t
sqc_str_parse_int32(const char *buf, int32_t *val);
sqc_result_t
sqc_str_parse_uint32_by_base(const char *buf, uint32_t *val,
                                unsigned int base);
sqc_result_t
sqc_str_parse_uint32(const char *buf, uint32_t *val);





sqc_result_t
sqc_str_parse_int64_by_base(const char *buf, int64_t *val,
                               unsigned int base);
sqc_result_t
sqc_str_parse_int64(const char *buf, int64_t *val);
sqc_result_t
sqc_str_parse_uint64_by_base(const char *buf, uint64_t *val,
                                unsigned int base);
sqc_result_t
sqc_str_parse_uint64(const char *buf, uint64_t *val);





sqc_result_t
sqc_str_parse_float(const char *buf, float *val);


sqc_result_t
sqc_str_parse_double(const char *buf, double *val);


sqc_result_t
sqc_str_parse_long_double(const char *buf, long double *val);


sqc_result_t
sqc_str_parse_bool(const char *buf, bool *val);





sqc_result_t
sqc_str_indexof(const char *str1, const char *str2);





__END_DECLS





#endif /* ! __SQC_STRUTILS_H__ */
