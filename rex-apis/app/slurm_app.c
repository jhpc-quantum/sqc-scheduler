#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "rexapis.h"

int main(int argc, char** argv) {
    int ret;

    uint32_t qc_type = 2;
    const char base_url[] = "";
    const char token[] = "";
    const char qprogram[] = "OPENQASM 2.0;\n"
                            "include \"qelib1.inc\";\n"
                            "qreg q[4];\n"
                            "rx(-0.785398163397448) q[1];\n";
    uint32_t circuit_fmt = 0;
    uint32_t shots = 1024;
    uint32_t transpiler = 2;
    const char remark[] = "test calculate";
    uint32_t polling_interval = 1000;
    uint32_t max_polling_count = 20;
    char *output = NULL;
    size_t output_len = 0;
    int *patterns = NULL;
    float *probs = NULL;
    size_t n_patterns = 0;

    printf("Slurm app start\n");

    char *qc_job_id = NULL;
    size_t qc_job_id_len = 0;
    char *err_msg = NULL;
    size_t err_msg_len = 0;
    ret = submit_job(qc_type, base_url, token, qprogram, circuit_fmt, shots, transpiler, remark,
                     &qc_job_id, &qc_job_id_len, &err_msg, &err_msg_len);
    if (ret == 0) {
        printf("submit_job succeeded: %s\n", qc_job_id);
    } else {
        printf("Failed to submit_job: %s\n", err_msg);
        fflush(stdout);
        return -1;
    }

//    ret = cancel_job(qc_type, base_url, token, qc_job_id, &err_msg, &err_msg_len);
//    if (ret == 0) {
//        printf("cancel_job succeeded: %s\n", qc_job_id);
//    } else {
//        printf("Failed to cancel_job: %s\n", err_msg);
//        fflush(stdout);
//        return -1;
//    }

    char *status = NULL;
    size_t status_len = 0;
    for (int i = 0; i < max_polling_count; i++) {
        ret = get_job_status(qc_type, base_url, token, qc_job_id,
                             &status, &status_len, &err_msg, &err_msg_len);
        if (ret == 0) {
            printf("get_job_status succeeded: %s\n", status);
            if (strcmp(status, "COMPLETED") == 0) {
                break;
            } else if (strcmp(status, "PENDING") == 0 ||
                       strcmp(status, "PREEMPTED") == 0 ||
                       strcmp(status, "RUNNING") == 0 ||
                       strcmp(status, "SUSPENDED") == 0) {
                usleep(polling_interval * 1000);
                continue;
            }
        }

        printf("Failed to get_job_status: %s\n", err_msg);
        fflush(stdout);
        return -1;
    }

    char *result = NULL;
    size_t result_len = 0;
    ret = get_job_result(qc_type, base_url, token, qc_job_id,
                         &result, &result_len, &err_msg, &err_msg_len);
    if (ret == 0) {
        printf("get_job_result succeeded: %s\n", result);
    } else {
        printf("Failed to get_job_result: %s\n", err_msg);
        fflush(stdout);
        return -1;
    }

    return 0;
}

