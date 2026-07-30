#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rexapis.h"

int main(int argc, char** argv) {
    int ret;

    uint32_t qc_type = 7;
    const char base_url[] = "";
    const char token[] = "";
    const char qprogram[] =
        "{"
        "\"name\":\"\","
        "\"description\":\"\","
        "\"device_id\":\"qulacs\","
        "\"shots\":1000,"
        "\"job_type\":\"sampling\","
        "\"job_info\":{"
            "\"program\":["
                "\"OPENQASM 3.0; include 'stdgates.inc'; qubit[2] qubits; bit[2] bits; h qubits[0]; cx qubits[0], qubits[1]; bits = measure qubits;\""
            "]"
        "},"
        "\"transpiler_info\":{},"
        "\"simulator_info\":{},"
        "\"mitigation_info\":{}"
        "}";
    uint32_t circuit_fmt = 3;
    uint32_t shots = 1024;
    uint32_t transpiler = 0;
    const char remark[] = "test OQTOPUS job";
    uint32_t polling_interval = 10000;
    uint32_t max_polling_count = 20;
    char *output = NULL;
    size_t output_len = 0;
    int *patterns = NULL;
    float *probs = NULL;
    size_t n_patterns = 0;

    printf("OQTOPUS app start\n");

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
            if (strcmp(status, "succeeded") == 0) {
                break;
            } else if (strcmp(status, "failed") == 0 ||
                       strcmp(status, "cancelled") == 0) {
                printf("Job execution is finished: %s\n", status);
                fflush(stdout);
                return -1;
            } else {
                free(status);
                status = NULL;
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

//    ret = delete_job(qc_type, base_url, token, qc_job_id, &err_msg, &err_msg_len);
//    if (ret == 0) {
//        printf("delete_job succeeded: %s\n", qc_job_id);
//    } else {
//        printf("Failed to cancel_job: %s\n", err_msg);
//        fflush(stdout);
//        return -1;
//    }

    free(qc_job_id);
    qc_job_id = NULL;
    free(err_msg);
    err_msg = NULL;
    free(status);
    status = NULL;
    free(result);
    result = NULL;

    return 0;
}

