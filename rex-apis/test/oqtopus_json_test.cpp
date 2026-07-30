#include "gtest/gtest.h"

#include "oqtopus_json.hpp"

namespace {

TEST(OQTOPUSSubmitJobResponseParser, Parse) {
    const std::string jsonStr = R"(
{
  "job_id":"dummy-id"
}
    )";
    const std::string expectedJobId = "dummy-id";

    rexapis::OQTOPUSSubmitJobResponseParser parser{};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::SubmitJobValue>(result)) {
        const rexapis::SubmitJobValue& val = std::get<rexapis::SubmitJobValue>(result);
        ASSERT_EQ(expectedJobId, val.GetQCJobId());
    } else if (std::holds_alternative<rexapis::JsonParseError>(result)) {
        const rexapis::JsonParseError& val = std::get<rexapis::JsonParseError>(result);
        FAIL() << val.GetMessage();
    } else {
        FAIL();
    }

}

TEST(OQTOPUSGetJobStatusResponseParser, Parse) {
    const std::string jobId = "dummy-id";
    const std::string jsonStr = R"(
{
  "job_id":"dummy-id",
  "status":"succeeded"
}
    )";
    const std::string expectedJobId = "dummy-id";
    const std::string expectedStatus = "succeeded";

    rexapis::OQTOPUSGetJobStatusResponseParser parser{jobId};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::GetJobStatusValue>(result)) {
        const rexapis::GetJobStatusValue& val = std::get<rexapis::GetJobStatusValue>(result);
        ASSERT_EQ(expectedJobId, val.GetQCJobId());
        ASSERT_EQ(expectedStatus, val.GetStatus());
    } else if (std::holds_alternative<rexapis::JsonParseError>(result)) {
        const rexapis::JsonParseError& val = std::get<rexapis::JsonParseError>(result);
        FAIL() << val.GetMessage();
    } else {
        FAIL();
    }
}

TEST(OQTOPUSGetJobResultResponseParser, Parse) {
    const std::string jobId = "dummy-id";
    const std::string jsonStr = R"(
{
  "job_id": "dummy-id",
  "name": "",
  "description": "",
  "job_type": "sampling",
  "status": "succeeded",
  "device_id": "qulacs",
  "shots": 1000,
  "job_info": {
    "program": [
      "OPENQASM 3.0; include 'stdgates.inc'; qubit[2] qubits; bit[2] bits; h qubits[0]; cx qubits[0], qubits[1]; bits = measure qubits;"
    ],
    "result": {
      "sampling": {
        "counts": {
          "11": 530,
          "00": 470
        }
      }
    },
    "transpile_result": {
      "transpiled_program": "OPENQASM 3.0;\ninclude \"stdgates.inc\";\nbit[2] bits;\nrz(pi/2) $0;\nsx $0;\nrz(pi/2) $0;\ncx $0, $1;\nbits[0] = measure $0;\nbits[1] = measure $1;\n",
      "stats": {
        "before": {
          "n_qubits": 2,
          "n_gates": 4,
          "n_gates_1q": 3,
          "n_gates_2q": 1,
          "depth": 3
        },
        "after": {
          "n_qubits": 16,
          "n_gates": 6,
          "n_gates_1q": 5,
          "n_gates_2q": 1,
          "depth": 5
        }
      },
      "virtual_physical_mapping": {
        "qubit_mapping": {
          "0": 0,
          "1": 1
        },
        "bit_mapping": {
          "0": 0,
          "1": 1
        }
      }
    },
    "message": "job is succeeded"
  },
  "transpiler_info": {
    "transpiler_lib": "qiskit",
    "transpiler_options": {
      "optimization_level": 1
    }
  },
  "simulator_info": {},
  "mitigation_info": {},
  "execution_time": 0.013,
  "submitted_at": "2026-05-11T00:40:16Z",
  "ready_at": "2026-05-11T00:40:17Z",
  "running_at": "2026-05-11T00:40:18Z",
  "ended_at": "2026-05-11T00:40:18Z"
}
    )";
    const std::string expectedJobId = "dummy-id";
    const std::string expectedResult = R"(
{
  "job_id": "dummy-id",
  "name": "",
  "description": "",
  "job_type": "sampling",
  "status": "succeeded",
  "device_id": "qulacs",
  "shots": 1000,
  "job_info": {
    "program": [
      "OPENQASM 3.0; include 'stdgates.inc'; qubit[2] qubits; bit[2] bits; h qubits[0]; cx qubits[0], qubits[1]; bits = measure qubits;"
    ],
    "result": {
      "sampling": {
        "counts": {
          "11": 530,
          "00": 470
        }
      }
    },
    "transpile_result": {
      "transpiled_program": "OPENQASM 3.0;\ninclude \"stdgates.inc\";\nbit[2] bits;\nrz(pi/2) $0;\nsx $0;\nrz(pi/2) $0;\ncx $0, $1;\nbits[0] = measure $0;\nbits[1] = measure $1;\n",
      "stats": {
        "before": {
          "n_qubits": 2,
          "n_gates": 4,
          "n_gates_1q": 3,
          "n_gates_2q": 1,
          "depth": 3
        },
        "after": {
          "n_qubits": 16,
          "n_gates": 6,
          "n_gates_1q": 5,
          "n_gates_2q": 1,
          "depth": 5
        }
      },
      "virtual_physical_mapping": {
        "qubit_mapping": {
          "0": 0,
          "1": 1
        },
        "bit_mapping": {
          "0": 0,
          "1": 1
        }
      }
    },
    "message": "job is succeeded"
  },
  "transpiler_info": {
    "transpiler_lib": "qiskit",
    "transpiler_options": {
      "optimization_level": 1
    }
  },
  "simulator_info": {},
  "mitigation_info": {},
  "execution_time": 0.013,
  "submitted_at": "2026-05-11T00:40:16Z",
  "ready_at": "2026-05-11T00:40:17Z",
  "running_at": "2026-05-11T00:40:18Z",
  "ended_at": "2026-05-11T00:40:18Z"
}
    )";

    rexapis::OQTOPUSGetJobResultResponseParser parser{jobId};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::GetJobResultValue>(result)) {
        const rexapis::GetJobResultValue& val = std::get<rexapis::GetJobResultValue>(result);
        ASSERT_EQ(expectedJobId, val.GetQCJobId());
        auto actualVal = json::value::parse(val.GetResult());
        auto expectedVal = json::value::parse(expectedResult);
        EXPECT_EQ(expectedVal, actualVal);
    } else if (std::holds_alternative<rexapis::JsonParseError>(result)) {
        const rexapis::JsonParseError& val = std::get<rexapis::JsonParseError>(result);
        FAIL() << val.GetMessage();
    } else {
        FAIL();
    }
}

TEST(OQTOPUSCancelJobResponseParser, Parse) {
    const std::string jobId = "dummy-id";
    const std::string jsonStr = R"(
{
  "message":"job canceled"
}
    )";
    const std::string expectedJobId = "dummy-id";
    const std::string expectedMsg = "job canceled";

    rexapis::OQTOPUSCancelJobResponseParser parser{jobId};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::CancelJobValue>(result)) {
        const rexapis::CancelJobValue& val = std::get<rexapis::CancelJobValue>(result);
        ASSERT_EQ(expectedJobId, val.GetQCJobId());
        ASSERT_EQ(expectedMsg, val.GetMsg());
    } else if (std::holds_alternative<rexapis::JsonParseError>(result)) {
        const rexapis::JsonParseError& val = std::get<rexapis::JsonParseError>(result);
        FAIL() << val.GetMessage();
    } else {
        FAIL();
    }
}

TEST(OQTOPUSDeleteJobResponseParser, Parse) {
    const std::string jobId = "dummy-id";
    const std::string jsonStr = R"(
{
  "message":"job deleted"
}
    )";
    const std::string expectedJobId = "dummy-id";
    const std::string expectedMsg = "job deleted";

    rexapis::OQTOPUSDeleteJobResponseParser parser{jobId};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::DeleteJobValue>(result)) {
        const rexapis::DeleteJobValue& val = std::get<rexapis::DeleteJobValue>(result);
        ASSERT_EQ(expectedJobId, val.GetQCJobId());
        ASSERT_EQ(expectedMsg, val.GetMsg());
    } else if (std::holds_alternative<rexapis::JsonParseError>(result)) {
        const rexapis::JsonParseError& val = std::get<rexapis::JsonParseError>(result);
        FAIL() << val.GetMessage();
    } else {
        FAIL();
    }
}

TEST(OQTOPUSRegisterJobResponseParser, Parse) {
    const std::string jsonStr = R"(
{
  "job_id": "dummy-id",
  "presigned_url": {
    "url": "dummy-url",
    "fields": {
      "key": "dummy-key",
      "AWSAccessKeyId": "dummy-AWSAccessKeyId",
      "x-amz-security-token": "dummy-x-amz-security-token",
      "policy": "dummy-policy",
      "signature": "dummy-signature"
    }
  }
}
    )";
    const std::string expectedQcJobId = "dummy-id";
    const std::string expectedUrl = "dummy-url";
    const std::string expectedkey = "dummy-key";
    const std::string expectedAwsAccessKeyId = "dummy-AWSAccessKeyId";
    const std::string expectedAmzSecurityToken = "dummy-x-amz-security-token";
    const std::string expectedPolicy = "dummy-policy";
    const std::string expectedSignature = "dummy-signature";

    rexapis::OQTOPUSRegisterJobResponseParser parser{};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::OQTOPUSRegisterJobValue>(result)) {
        const rexapis::OQTOPUSRegisterJobValue& val = std::get<rexapis::OQTOPUSRegisterJobValue>(result);
        ASSERT_EQ(expectedQcJobId, val.GetQCJobId());
        ASSERT_EQ(expectedUrl, val.GetUrl());
        ASSERT_EQ(expectedkey, val.GetKey());
        ASSERT_EQ(expectedAwsAccessKeyId, val.GetAwsAccessKeyId());
        ASSERT_EQ(expectedAmzSecurityToken, val.GetAmzSecurityToken());
        ASSERT_EQ(expectedPolicy, val.GetPolicy());
        ASSERT_EQ(expectedSignature, val.GetSignature());
    } else if (std::holds_alternative<rexapis::JsonParseError>(result)) {
        const rexapis::JsonParseError& val = std::get<rexapis::JsonParseError>(result);
        FAIL() << val.GetMessage();
    } else {
        FAIL();
    }
}

}

