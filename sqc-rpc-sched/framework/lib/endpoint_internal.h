#pragma once





typedef struct sqc_endpoint_info_reciord {
  bool inited_;
  int family_;
  char hint_addr_str_[SQC_HOST_NAME_MAX + 1];
  char addr_fqdn_str_[SQC_HOST_NAME_MAX + 1];
  char addr_num_str_[SQC_HOST_NAME_MAX + 1];
  int port_;
  socklen_t sockaddr_len_;
  struct sockaddr_storage sockaddr_;
} sqc_endpoint_info;


typedef struct sqc_endpoint_record {
  sqc_endpoint_type_t type_;
  int fd_;

  sqc_endpoint_info local_;
  sqc_endpoint_info peer_;

  bool is_connected_;
  bool is_bound_;
  bool is_listening_;
  bool is_accepted_;

  bool is_nodelay_;
  bool is_nonblocking_;

  int org_fcntl_mode_;
} sqc_endpoint_record;



