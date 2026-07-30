#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "rexapis.h"

int main(int argc, char** argv) {
    int ret;

    uint32_t qc_type = 1;
    const char base_url[] = "";
    const char token[] = "";
    const char qprogram[] = "OPENQASM 3; include \"stdgates.inc\"; qreg q[4]; creg c[4]; h q[0]; h q[1]; ccx q[0],q[1],q[2]; cx q[0],q[3]; cx q[1],q[3]; measure q[3] -> c[0]; measure q[2] -> c[1]; measure q[1] -> c[2]; measure q[0] -> c[3];";
    uint32_t circuit_fmt = 0;
    uint32_t shots = 1024;
    uint32_t transpiler = 0;
    const char remark[] = "test calculate";
    uint32_t polling_interval = 300000;
    uint32_t max_polling_count = 20;
    char *output = NULL;
    size_t output_len = 0;
    int *patterns = NULL;
    float *probs = NULL;
    size_t n_patterns = 0;

    printf("IBM Q app start\n");

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
            if (strcmp(status, "Completed") == 0) {
                break;
            } else if (strcmp(status, "Failed") == 0 ||
                       strcmp(status, "Cancelled") == 0) {
                printf("Job execution is finished: %s\n", status);
                fflush(stdout);
                return -1;
            } else {
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

