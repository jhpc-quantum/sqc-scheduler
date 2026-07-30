#include <stdio.h>
#include <stdint.h>

#include "rexapis.h"

int main(int argc, char** argv) {
    int ret;

    uint32_t qc_type = 0;
    const char base_url[] = "";
    const char token[] = "";
    const char qprogram[] = "OPENQASM 3; include \"stdgates.inc\"; qreg q[4]; creg c[4]; h q[0]; h q[1]; ccx q[0],q[1],q[2]; cx q[0],q[3]; cx q[1],q[3]; measure q[3] -> c[0]; measure q[2] -> c[1]; measure q[1] -> c[2]; measure q[0] -> c[3];";
    uint32_t circuit_fmt = 0;
    uint32_t shots = 1024;
    uint32_t transpiler = 2;
    const char remark[] = "test calculate";
    uint32_t polling_interval = 1000;
    uint32_t max_polling_count = 60;
    char *output = NULL;
    size_t output_len = 0;
    int *patterns = NULL;
    float *probs = NULL;
    size_t n_patterns = 0;

    printf("calculate RQC\n");

    ret = calculate(qc_type, base_url, token, qprogram, circuit_fmt, shots, transpiler, remark,
                    polling_interval, max_polling_count, &output, &output_len);
    if (ret != 0) {
        printf("RQC App: Failed to calculate.\n");
        fflush(stdout);
        return -1;
    }

    printf("RQC App: REST result %s\n", output);

    ret = scrape_response(qc_type, output, shots, &patterns, &probs, &n_patterns);
    if (ret != 0) {
        printf("RQC App result: Failed to scrape.\n");
        fflush(stdout);
        return -1;
    }

    printf("RQC App: result size=%ld\n", n_patterns);
    for (int i = 0; i < n_patterns; i++) {
        printf("RQC App: [%d] %d, %f\n", i, patterns[i], probs[i]);
    }
    fflush(stdout);
    return 0;
}

