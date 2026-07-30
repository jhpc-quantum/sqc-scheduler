#include "gtest/gtest.h"

#include "rqc_json.hpp"

namespace {

static const std::string kJsonStr = R"(
{
  "id": "ID",
  "qasm": "OPENQASM 3;",
  "shots": 1024,
  "transpiler": "normal",
  "status": "success",
  "result": "{\n  \"counts\": {\n    \"0011\": 262,\n    \"0100\": 243,\n    \"1001\": 260,\n    \"1010\": 259\n  },\n  \"properties\": {\n    \"0\": {\n      \"qubit_index\": 0,\n      \"measurement_window_index\": 0\n    },\n    \"1\": {\n      \"qubit_index\": 1,\n      \"measurement_window_index\": 0\n    },\n    \"2\": {\n      \"qubit_index\": 2,\n      \"measurement_window_index\": 0\n    },\n    \"3\": {\n      \"qubit_index\": 3,\n      \"measurement_window_index\": 0\n    }\n  },\n  \"transpiler_info\": {\n    \"physical_virtual_mapping\": {\n      \"0\": 3,\n      \"1\": 1,\n      \"2\": 0,\n      \"3\": 2\n    }\n  },\n  \"message\": \"SUCCESS!\"\n}\n",
  "transpiled_qasm": "transpiled qasm",
  "remark": "test calculate",
  "in_queue": "2024-01-05 14:59:20",
  "out_queue": "2024-01-05 05:59:26",
  "created": "2024-01-05 14:59:20",
  "ended": "2024-01-05 05:59:27"
}
)";
static const std::uint32_t kShots = 4096;

TEST(RQCJsonParser, ScrapeResponse) {
    std::shared_ptr<rexapis::JsonResult> result = rexapis::RQCJsonParser::ScrapeResponse(kJsonStr, kShots);

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

TEST(RQCJsonParser, Scrape) {
    std::shared_ptr<rexapis::RQCJsonParser> parser = std::make_shared<rexapis::RQCJsonParser>(kJsonStr, kShots);
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

}

