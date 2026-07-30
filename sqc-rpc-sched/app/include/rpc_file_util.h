#pragma once


/**
 * @file    rpc_file_util.h
 */

__BEGIN_DECLS


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

///
/// @brief   Read a text file.
///
/// @param[in]     path          A path to a file.
/// @param[out]    out_path      An expaned path to the file.
///
/// @retval SQC_RESULT_OK                    Succeeded.
/// @retval SQC_RESULT_NO_MEMORY             Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS          Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES          Failed, any other reason.
///
/// @details The function <tt>rpc_expand_path()</tt> expands \c path.
/// If \c path starts with '~/' or '~username/', it replaces it with a path to
/// the home directory of that user as shells do.
/// The caller is responsible for freeing \c out_path.
///
sqc_result_t
rpc_expand_path(const char* path, char **out_path);


///
/// @brief   Read a text file.
///
/// @param[in]     id           A path to the text file.
/// @param[out]    text         The contents of the loaded text file.
/// @param[out]    textlen      Length of \c text including NUL.
///
/// @retval SQC_RESULT_OK                    Succeeded.
/// @retval SQC_RESULT_NO_MEMORY             Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS          Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES          Failed, any other reason.
///
/// @details The function <tt>rpc_read_text_file()</tt> reads the entire text file.
/// Upon success, \c buf points to text data read from the file and \c textlen represents
/// length of the text data.  \c buf is terminated with NUL character and \c textlen
/// counts the NUL character.  The caller is responsible for freeing \c text.
///
sqc_result_t
rpc_read_text_file(const char* file, char **text, size_t *textlen);


///
/// @brief   Strip whitespaces at the end of the strig.
///
/// @param[in.out] str          A string.
///
/// @details The function <tt>rpc_strip_text()</tt> strips whitespaces
/// (' ', '\t', '\r' and '\n') at the end of \c str.
///
void
rpc_strip_text(char *str);



#ifdef __cplusplus
}
#endif // __cplusplus


__END_DECLS
