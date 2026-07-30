#include "gtest/gtest.h"

#include "ibmq_json.hpp"

namespace {

static const std::string kJsonStr = R"(
{
  "quasi_dists": [
    {
      "0000": 0.2578125,
      "0101": 0.2333984375,
      "1001": 0.2529296875,
      "1110": 0.255859375
    }
  ],
  "metadata": [
    {
      "shots": 1024,
      "circuit_metadata": {},
      "readout_mitigation_overhead": 1.0,
      "readout_mitigation_time": 0.00478166900575161
    }
  ]
}
)";
static const std::uint32_t kShots = 4096;

TEST(IBMQJsonParser, ScrapeResponse) {
    const std::string jsonStr = R"(
{
  "results": [
    {
      "data": {
        "c": {
          "samples": [
            "0x1",
            "0x1",
            "0x1",
            "0x1",
            "0x1",
            "0x1",
            "0x1",
            "0x1",
            "0x1",
            "0x1"
          ],
          "num_bits": 1
        }
      },
      "metadata": {
        "circuit_metadata": {}
      }
    }
  ],
  "metadata": {
    "execution": {
      "execution_spans": [
        [
          {
            "date": "2025-06-24T04:58:09.556210"
          },
          {
            "date": "2025-06-24T04:58:10.322832"
          },
          {
            "0": [
              [
                10
              ],
              [
                0,
                1
              ],
              [
                0,
                10
              ]
            ]
          }
        ]
      ]
    },
    "version": 2
  }
}
    )";
    const std::uint32_t shots = 4096;
    std::shared_ptr<rexapis::JsonResult> result = rexapis::IBMQJsonParser::ScrapeResponse(jsonStr, shots);

    std::vector<int> patterns = result->GetPatterns();
    std::vector<float> probs = result->GetProbs();
    std::uint32_t size = result->GetSize();

    std::wcout << "size   : " << size << std::endl;

    for (int i = 0; i < size; i++) {
        auto pattern = patterns[i];
        auto prob = probs[i];

        std::wcout << "pattern: " << pattern << std::endl;
        std::wcout << "prob   : " << prob << std::endl;
    }
}

TEST(IBMQJsonParser, Scrape) {
    const std::string jsonStr = R"(
{
  "results": [
    {
      "data": {
        "c": {
          "samples": [
            "0x1",
            "0x1",
            "0x1",
            "0x1",
            "0x1",
            "0x1",
            "0x1",
            "0x1",
            "0x1",
            "0x1"
          ],
          "num_bits": 1
        }
      },
      "metadata": {
        "circuit_metadata": {}
      }
    }
  ],
  "metadata": {
    "execution": {
      "execution_spans": [
        [
          {
            "date": "2025-06-24T04:58:09.556210"
          },
          {
            "date": "2025-06-24T04:58:10.322832"
          },
          {
            "0": [
              [
                10
              ],
              [
                0,
                1
              ],
              [
                0,
                10
              ]
            ]
          }
        ]
      ]
    },
    "version": 2
  }
}
    )";
    const std::uint32_t shots = 4096;
    std::shared_ptr<rexapis::IBMQJsonParser> parser = std::make_shared<rexapis::IBMQJsonParser>(jsonStr, shots);
    std::shared_ptr<rexapis::JsonResult> result = parser->Scrape();

    std::vector<int> patterns = result->GetPatterns();
    std::vector<float> probs = result->GetProbs();
    std::uint32_t size = result->GetSize();

    std::wcout << "size   : " << size << std::endl;

    for (int i = 0; i < size; i++) {
        auto pattern = patterns[i];
        auto prob = probs[i];

        std::wcout << "pattern: " << pattern << std::endl;
        std::wcout << "prob   : " << prob << std::endl;
    }
}

TEST(IBMQGetJobStatusResponseParser, Parse) {
    const std::string jobId = "dummy-id";
    const std::string jsonStr = R"(
{
  "id": "dummy-id",
  "hub": "ibm-q",
  "group": "open",
  "project": "main",
  "backend": "vendor_location",
  "state": {
    "status": "Completed"
  },
  "status": "Completed",
  "params": {
    "pubs": [
      [
        "OPENQASM 3.0; include \"stdgates.inc\"; bit[1] c; x $47; c[0] = measure $47;"
      ]
    ],
    "shots": 10,
    "version": 2
  },
  "program": {
    "id": "sampler"
  },
  "created": "2025-06-24T04:58:04.675Z",
  "ended": "2025-06-24T04:58:11.088Z",
  "runtime": "",
  "cost": 600,
  "tags": null,
  "session_id": null,
  "usage": {
    "seconds": 0
  },
  "private": false,
  "user_id": "dummy-user_id"
}
    )";
    const std::string expectedJobId = "dummy-id";
    const std::string expectedStatus = "Completed";

    rexapis::IBMQGetJobStatusResponseParser parser{jobId};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::GetJobStatusValue>(result)) {
        const rexapis::GetJobStatusValue& val = std::get<rexapis::GetJobStatusValue>(result);
        ASSERT_EQ(expectedJobId, val.GetQCJobId());
        ASSERT_EQ(expectedStatus, val.GetStatus());
    } else {
        FAIL();
    }
}

TEST(IBMQTranspileCircuitResponseParser, Parse) {
    const std::string jsonStr = R"(
{
  "task_id": "0000000-0000-1111-2222-333333333333"
}
    )";
    const std::string expectedTaskId = "0000000-0000-1111-2222-333333333333";

    rexapis::IBMQTranspileCircuitResponseParser parser{jsonStr};
    auto result = parser.Parse();
    if (std::holds_alternative<rexapis::IBMQTranspileCircuitValue>(result)) {
        const rexapis::IBMQTranspileCircuitValue& val = std::get<rexapis::IBMQTranspileCircuitValue>(result);
        ASSERT_EQ(expectedTaskId, val.GetTaskId());
    } else {
        FAIL();
    }
}

TEST(IBMQGetTranspilationResultsResponseParser, Parse) {
    const std::string taskId = "0000000-0000-1111-2222-333333333333";
    const std::string jsonStr = R"(
{
  "state": "SUCCESS",
  "result": [
    {
      "qasm": "OPENQASM 3.0; include \"stdgates.inc\"; bit[1] c; x $47; c[0] = measure $47;",
      "qpy": null,
      "success": true,
      "layout": {
        "initial": [
          47,
          0,
          1
        ],
        "final": [
          47,
          0,
          1
        ]
      },
      "error": null
    }
  ]
}
    )";
    const std::string expectedTaskId = "0000000-0000-1111-2222-333333333333";
    const std::string expectedState = "SUCCESS";
    const std::string expectedResult = "OPENQASM 3.0; include \"stdgates.inc\"; bit[1] c; x $47; c[0] = measure $47;";

    rexapis::IBMQGetTranspilationResultsResponseParser parser{taskId, jsonStr};
    auto result = parser.Parse();
    if (std::holds_alternative<rexapis::IBMQGetTranspilationResultsValue>(result)) {
        const rexapis::IBMQGetTranspilationResultsValue& val = std::get<rexapis::IBMQGetTranspilationResultsValue>(result);
        ASSERT_EQ(expectedTaskId, val.GetTaskId());
        ASSERT_EQ(expectedState, val.GetState());
        ASSERT_EQ(expectedResult, val.GetResult());
    } else {
        FAIL();
    }
}

}

