#ifndef __SQC_IP_ADDR_H__
#define __SQC_IP_ADDR_H__

#define SQC_ADDR_STR_MAX NI_MAXHOST





__BEGIN_DECLS





/**
 * @brief	sqc_ip_address_t
 */
typedef struct ip_address sqc_ip_address_t;

/**
 * Create a sqc_ip_address_t.
 *
 *     @param[in]	name	Host name.
 *     @param[in]	is_ipv4_addr	IPv4 flag.
 *     @param[out]	ip	A pointer to a \e sqc_ip_address_t structure.
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	SQC_RESULT_NO_MEMORY	Failed, no memory.
 */
sqc_result_t
sqc_ip_address_create(const char *name, bool is_ipv4_addr,
                         sqc_ip_address_t **ip);

/**
 * Destroy a sqc_ip_address_t
 *
 *     @param[in]	ip	A pointer to a \e sqc_ip_address_t structure.
 *
 *     @retval	void
 */
void
sqc_ip_address_destroy(sqc_ip_address_t *ip);

/**
 * Copy a sqc_ip_address_t.
 *
 *     @param[in]	name	Host name.
 *     @param[in]	src	A pointer to a \e sqc_ip_address_t structure (src).
 *     @param[out]	dst	A pointer to a \e sqc_ip_address_t structure (dst).
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	SQC_RESULT_NO_MEMORY	Failed, no memory.
 */
sqc_result_t
sqc_ip_address_copy(const sqc_ip_address_t *src,
                       sqc_ip_address_t **dst);

/**
 * Equals a sqc_ip_address_t.
 *
 *     @param[in]	ip1	A pointer to a \e sqc_ip_address_t structure (src).
 *     @param[in]	ip2	A pointer to a \e sqc_ip_address_t structure (dst).
 *
 *     @retval  true/false
 */
bool
sqc_ip_address_equals(const sqc_ip_address_t *ip1,
                         const sqc_ip_address_t *ip2);

/**
 * Get IP addr string.
 *
 *     @param[in]	ip	A pointer to a \e sqc_ip_address_t structure.
 *     @param[out]	addr_str	IP addr string..
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	SQC_RESULT_NO_MEMORY	Failed, no memory.
 */
sqc_result_t
sqc_ip_address_str_get(const sqc_ip_address_t *ip,
                          char **addr_str);

/**
 * Get sockaddr structure.
 *
 *     @param[in]	ip	A pointer to a \e sqc_ip_address_t structure.
 *     @param[out]	saddr	A pointer to a \e sockaddr structure
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	SQC_RESULT_NO_MEMORY	Failed, no memory.
 */
sqc_result_t
sqc_ip_address_sockaddr_get(const sqc_ip_address_t *ip,
                               struct sockaddr **saddr);

/**
 * Get length of sockaddr structure.
 *
 *     @param[in]	ip	A pointer to a \e sqc_ip_address_t structure.
 *     @param[out]	saddr_len	A pointer to a \e length of sockaddr structure
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
sqc_result_t
sqc_ip_address_sockaddr_len_get(const sqc_ip_address_t *ip,
                                   socklen_t *saddr_len);


/**
 * Is IPv4.
 *
 *     @param[in]	ip	A pointer to a \e sqc_ip_address_t structure.
 *     @param[out]	is_ipv4	A pointer to a \e is_ipv4.
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
sqc_result_t
sqc_ip_address_is_ipv4(const sqc_ip_address_t *ip,
                          bool *is_ipv4);





__END_DECLS





#endif /* __SQC_IP_ADDR_H__ */
