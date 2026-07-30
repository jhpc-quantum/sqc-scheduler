#include "gtest/gtest.h"

#include "slurm_json.hpp"

namespace {

TEST(SlurmJsonParser, ScrapeResponse) {
    const std::string jsonStr = R"(
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
    const std::uint32_t shots = 4096;

    std::shared_ptr<rexapis::JsonResult> result = rexapis::SlurmJsonParser::ScrapeResponse(jsonStr, shots);

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

TEST(SlurmJsonParser, Scrape) {
    const std::string jsonStr = R"(
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
    const std::uint32_t shots = 4096;

    std::shared_ptr<rexapis::SlurmJsonParser> parser = std::make_shared<rexapis::SlurmJsonParser>(jsonStr, shots);
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

TEST(SlurmSubmitJobResponseParser, Parse) {
    const std::string jsonStr = R"(
{
  "result": {
    "job_id": 333,
    "step_id": "batch",
    "error_code": 0,
    "error": "No error",
    "job_submit_user_msg": ""
  },
  "job_id": 333,
  "step_id": "batch",
  "job_submit_user_msg": "",
  "meta": {
    "plugin": {
      "type": "openapi\/slurmctld",
      "name": "Slurm OpenAPI slurmctld",
      "data_parser": "data_parser\/v0.0.41",
      "accounting_storage": "accounting_storage\/slurmdbd"
    },
    "client": {
      "source": "[hostname]:43210",
      "user": "root",
      "group": "root"
    },
    "command": [
    ],
    "slurm": {
      "version": {
        "major": "24",
        "micro": "3",
        "minor": "05"
      },
      "release": "24.05.3",
      "cluster": "cluster"
    }
  },
  "errors": [
  ],
  "warnings": [
  ]
}
    )";
    const std::string expectedJobId = "333";

    rexapis::SlurmSubmitJobResponseParser parser{};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::SubmitJobValue>(result)) {
        const rexapis::SubmitJobValue& val = std::get<rexapis::SubmitJobValue>(result);
        ASSERT_EQ(expectedJobId, val.GetQCJobId());
    } else {
        FAIL();
    }
}

TEST(SlurmSubmitJobResponseParser, ParseInvalidJson) {
    const std::string jsonStr = R"(
{
  "result": {
    "job_id": 333,
  ]
    )";
    const std::string expectedMessage = "* Line 5, Column 3 Syntax error: Malformed object literal";

    rexapis::SlurmSubmitJobResponseParser parser{};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::JsonParseError>(result)) {
        const rexapis::JsonParseError& err = std::get<rexapis::JsonParseError>(result);
        ASSERT_EQ(expectedMessage, err.GetMessage());
    } else {
        FAIL();
    }
}

TEST(SlurmSubmitJobResponseParser, ParseEmptyJson) {
    const std::string jsonStr = "";
    const std::string expectedMessage = "* Line 1, Column 1 Syntax error: Malformed token";

    rexapis::SlurmSubmitJobResponseParser parser{};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::JsonParseError>(result)) {
        const rexapis::JsonParseError& err = std::get<rexapis::JsonParseError>(result);
        ASSERT_EQ(expectedMessage, err.GetMessage());
    } else {
        FAIL();
    }
}


TEST(SlurmGetJobStatusResponseParser, Parse) {
    const std::string jobId = "333";
    const std::string jsonStr = R"(
 {
  "jobs": [
    {
      "account": "",
      "accrue_time": {
        "set": true,
        "infinite": false,
        "number": 1746693311
      },
      "admin_comment": "",
      "allocating_node": "localhost",
      "array_job_id": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "array_task_id": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "array_max_tasks": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "array_task_string": "",
      "association_id": 0,
      "batch_features": "",
      "batch_flag": true,
      "batch_host": "hostname1",
      "flags": [
        "USING_DEFAULT_ACCOUNT",
        "USING_DEFAULT_QOS",
        "USING_DEFAULT_WCKEY"
      ],
      "burst_buffer": "",
      "burst_buffer_state": "",
      "cluster": "cluster",
      "cluster_features": "",
      "command": "",
      "comment": "",
      "container": "",
      "container_id": "",
      "contiguous": false,
      "core_spec": 0,
      "thread_spec": 32766,
      "cores_per_socket": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "billable_tres": {
        "set": true,
        "infinite": false,
        "number": 1.0
      },
      "cpus_per_task": {
        "set": true,
        "infinite": false,
        "number": 1
      },
      "cpu_frequency_minimum": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "cpu_frequency_maximum": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "cpu_frequency_governor": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "cpus_per_tres": "",
      "cron": "",
      "deadline": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "delay_boot": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "dependency": "",
      "derived_exit_code": {
        "status": [
          "SUCCESS"
        ],
        "return_code": {
          "set": true,
          "infinite": false,
          "number": 0
        },
        "signal": {
          "id": {
            "set": false,
            "infinite": false,
            "number": 0
          },
          "name": ""
        }
      },
      "eligible_time": {
        "set": true,
        "infinite": false,
        "number": 1746693311
      },
      "end_time": {
        "set": true,
        "infinite": false,
        "number": 1746693312
      },
      "excluded_nodes": "",
      "exit_code": {
        "status": [
          "SUCCESS"
        ],
        "return_code": {
          "set": true,
          "infinite": false,
          "number": 0
        },
        "signal": {
          "id": {
            "set": false,
            "infinite": false,
            "number": 0
          },
          "name": ""
        }
      },
      "extra": "",
      "failed_node": "",
      "features": "",
      "federation_origin": "",
      "federation_siblings_active": "",
      "federation_siblings_viable": "",
      "gres_detail": [
      ],
      "group_id": 1000,
      "group_name": "group",
      "het_job_id": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "het_job_id_set": "",
      "het_job_offset": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "job_id": 333,
      "job_resources": {
        "select_type": [
          "CORE"
        ],
        "nodes": {
          "count": 1,
          "select_type": [
            "ONE_ROW"
          ],
          "list": "hostname",
          "whole": false,
          "allocation": [
            {
              "index": 0,
              "name": "hostname",
              "cpus": {
                "count": 1,
                "used": 0
              },
              "memory": {
                "used": 0,
                "allocated": 15737
              },
              "sockets": [
                {
                  "index": 0,
                  "cores": [
                    {
                      "index": 0,
                      "status": [
                        "ALLOCATED"
                      ]
                    }
                  ]
                },
                {
                  "index": 1,
                  "cores": [
                    {
                      "index": 0,
                      "status": [
                        "UNALLOCATED"
                      ]
                    }
                  ]
                },
                {
                  "index": 2,
                  "cores": [
                    {
                      "index": 0,
                      "status": [
                        "UNALLOCATED"
                      ]
                    }
                  ]
                },
                {
                  "index": 3,
                  "cores": [
                    {
                      "index": 0,
                      "status": [
                        "UNALLOCATED"
                      ]
                    }
                  ]
                }
              ]
            }
          ]
        },
        "cpus": 1,
        "threads_per_core": {
          "set": false,
          "infinite": false,
          "number": 0
        }
      },
      "job_size_str": [
      ],
      "job_state": [
        "COMPLETED"
      ],
      "last_sched_evaluation": {
        "set": true,
        "infinite": false,
        "number": 1746693312
      },
      "licenses": "",
      "mail_type": [
      ],
      "mail_user": "qc-rest",
      "max_cpus": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "max_nodes": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "mcs_label": "",
      "memory_per_tres": "",
      "name": "REX REST JOB",
      "network": "",
      "nodes": "node1",
      "nice": 0,
      "tasks_per_core": {
        "set": false,
        "infinite": true,
        "number": 0
      },
      "tasks_per_tres": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "tasks_per_node": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "tasks_per_socket": {
        "set": false,
        "infinite": true,
        "number": 0
      },
      "tasks_per_board": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "cpus": {
        "set": true,
        "infinite": false,
        "number": 1
      },
      "node_count": {
        "set": true,
        "infinite": false,
        "number": 1
      },
      "tasks": {
        "set": true,
        "infinite": false,
        "number": 1
      },
      "partition": "rocky",
      "prefer": "",
      "memory_per_cpu": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "memory_per_node": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "minimum_cpus_per_node": {
        "set": true,
        "infinite": false,
        "number": 1
      },
      "minimum_tmp_disk_per_node": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "power": {
        "flags": [
        ]
      },
      "preempt_time": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "preemptable_time": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "pre_sus_time": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "hold": false,
      "priority": {
        "set": true,
        "infinite": false,
        "number": 1
      },
      "profile": [
        "NOT_SET"
      ],
      "qos": "normal",
      "reboot": false,
      "required_nodes": "",
      "minimum_switches": 0,
      "requeue": true,
      "resize_time": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "restart_cnt": 0,
      "resv_name": "",
      "scheduled_nodes": "",
      "selinux_context": "",
      "shared": [
      ],
      "exclusive": [
      ],
      "oversubscribe": true,
      "show_flags": [
        "ALL",
        "DETAIL",
        "LOCAL"
      ],
      "sockets_per_board": 0,
      "sockets_per_node": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "start_time": {
        "set": true,
        "infinite": false,
        "number": 1746693312
      },
      "state_description": "",
      "state_reason": "None",
      "standard_error": "\/home\/user\/test\/results\/myjob-308.stderr",
      "standard_input": "\/dev\/null",
      "standard_output": "\/home\/user\/test\/results\/myjob-308.stdout",
      "submit_time": {
        "set": true,
        "infinite": false,
        "number": 1746693311
      },
      "suspend_time": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "system_comment": "",
      "time_limit": {
        "set": false,
        "infinite": true,
        "number": 0
      },
      "time_minimum": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "threads_per_core": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "tres_bind": "",
      "tres_freq": "",
      "tres_per_job": "",
      "tres_per_node": "",
      "tres_per_socket": "",
      "tres_per_task": "",
      "tres_req_str": "cpu=1,mem=15737M,node=1,billing=1",
      "tres_alloc_str": "cpu=1,mem=15737M,node=1,billing=1",
      "user_id": 1000,
      "user_name": "qc-rest",
      "maximum_switch_wait_time": 0,
      "wckey": "",
      "current_working_directory": "\/home\/user\/test"
    }
  ],
  "last_backfill": {
    "set": true,
    "infinite": false,
    "number": 1746670919
  },
  "last_update": {
    "set": true,
    "infinite": false,
    "number": 1746693325
  },
  "meta": {
    "plugin": {
      "type": "openapi\/slurmctld",
      "name": "Slurm OpenAPI slurmctld",
      "data_parser": "data_parser\/v0.0.41",
      "accounting_storage": "accounting_storage\/slurmdbd"
    },
    "client": {
      "source": "[hostname]:9876",
      "user": "root",
      "group": "root"
    },
    "command": [
    ],
    "slurm": {
      "version": {
        "major": "24",
        "micro": "3",
        "minor": "05"
      },
      "release": "24.05.3",
      "cluster": "cluster"
    }
  },
  "errors": [
  ],
  "warnings": [
  ]
}
    )";
    const std::string expectedJobId = "333";
    const std::string expectedStatus = "COMPLETED";

    rexapis::SlurmGetJobStatusResponseParser parser{jobId};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::GetJobStatusValue>(result)) {
        const rexapis::GetJobStatusValue& val = std::get<rexapis::GetJobStatusValue>(result);
        ASSERT_EQ(expectedJobId, val.GetQCJobId());
        ASSERT_EQ(expectedStatus, val.GetStatus());
    } else {
        FAIL();
    }
}

TEST(SlurmGetJobStatusResponseParser, ParseJobIdNotFound) {
    const std::string jobId = "333";
    const std::string jsonStr = R"(
 {
  "jobs": [
    {
      "account": "",
      "accrue_time": {
        "set": true,
        "infinite": false,
        "number": 1746693311
      },
      "admin_comment": "",
      "allocating_node": "localhost",
      "array_job_id": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "array_task_id": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "array_max_tasks": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "array_task_string": "",
      "association_id": 0,
      "batch_features": "",
      "batch_flag": true,
      "batch_host": "hostname1",
      "flags": [
        "USING_DEFAULT_ACCOUNT",
        "USING_DEFAULT_QOS",
        "USING_DEFAULT_WCKEY"
      ],
      "burst_buffer": "",
      "burst_buffer_state": "",
      "cluster": "cluster",
      "cluster_features": "",
      "command": "",
      "comment": "",
      "container": "",
      "container_id": "",
      "contiguous": false,
      "core_spec": 0,
      "thread_spec": 32766,
      "cores_per_socket": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "billable_tres": {
        "set": true,
        "infinite": false,
        "number": 1.0
      },
      "cpus_per_task": {
        "set": true,
        "infinite": false,
        "number": 1
      },
      "cpu_frequency_minimum": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "cpu_frequency_maximum": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "cpu_frequency_governor": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "cpus_per_tres": "",
      "cron": "",
      "deadline": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "delay_boot": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "dependency": "",
      "derived_exit_code": {
        "status": [
          "SUCCESS"
        ],
        "return_code": {
          "set": true,
          "infinite": false,
          "number": 0
        },
        "signal": {
          "id": {
            "set": false,
            "infinite": false,
            "number": 0
          },
          "name": ""
        }
      },
      "eligible_time": {
        "set": true,
        "infinite": false,
        "number": 1746693311
      },
      "end_time": {
        "set": true,
        "infinite": false,
        "number": 1746693312
      },
      "excluded_nodes": "",
      "exit_code": {
        "status": [
          "SUCCESS"
        ],
        "return_code": {
          "set": true,
          "infinite": false,
          "number": 0
        },
        "signal": {
          "id": {
            "set": false,
            "infinite": false,
            "number": 0
          },
          "name": ""
        }
      },
      "extra": "",
      "failed_node": "",
      "features": "",
      "federation_origin": "",
      "federation_siblings_active": "",
      "federation_siblings_viable": "",
      "gres_detail": [
      ],
      "group_id": 1000,
      "group_name": "group",
      "het_job_id": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "het_job_id_set": "",
      "het_job_offset": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "job_id": 999,
      "job_resources": {
        "select_type": [
          "CORE"
        ],
        "nodes": {
          "count": 1,
          "select_type": [
            "ONE_ROW"
          ],
          "list": "hostname",
          "whole": false,
          "allocation": [
            {
              "index": 0,
              "name": "hostname",
              "cpus": {
                "count": 1,
                "used": 0
              },
              "memory": {
                "used": 0,
                "allocated": 15737
              },
              "sockets": [
                {
                  "index": 0,
                  "cores": [
                    {
                      "index": 0,
                      "status": [
                        "ALLOCATED"
                      ]
                    }
                  ]
                },
                {
                  "index": 1,
                  "cores": [
                    {
                      "index": 0,
                      "status": [
                        "UNALLOCATED"
                      ]
                    }
                  ]
                },
                {
                  "index": 2,
                  "cores": [
                    {
                      "index": 0,
                      "status": [
                        "UNALLOCATED"
                      ]
                    }
                  ]
                },
                {
                  "index": 3,
                  "cores": [
                    {
                      "index": 0,
                      "status": [
                        "UNALLOCATED"
                      ]
                    }
                  ]
                }
              ]
            }
          ]
        },
        "cpus": 1,
        "threads_per_core": {
          "set": false,
          "infinite": false,
          "number": 0
        }
      },
      "job_size_str": [
      ],
      "job_state": [
        "COMPLETED"
      ],
      "last_sched_evaluation": {
        "set": true,
        "infinite": false,
        "number": 1746693312
      },
      "licenses": "",
      "mail_type": [
      ],
      "mail_user": "qc-rest",
      "max_cpus": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "max_nodes": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "mcs_label": "",
      "memory_per_tres": "",
      "name": "REX REST JOB",
      "network": "",
      "nodes": "node1",
      "nice": 0,
      "tasks_per_core": {
        "set": false,
        "infinite": true,
        "number": 0
      },
      "tasks_per_tres": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "tasks_per_node": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "tasks_per_socket": {
        "set": false,
        "infinite": true,
        "number": 0
      },
      "tasks_per_board": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "cpus": {
        "set": true,
        "infinite": false,
        "number": 1
      },
      "node_count": {
        "set": true,
        "infinite": false,
        "number": 1
      },
      "tasks": {
        "set": true,
        "infinite": false,
        "number": 1
      },
      "partition": "rocky",
      "prefer": "",
      "memory_per_cpu": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "memory_per_node": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "minimum_cpus_per_node": {
        "set": true,
        "infinite": false,
        "number": 1
      },
      "minimum_tmp_disk_per_node": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "power": {
        "flags": [
        ]
      },
      "preempt_time": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "preemptable_time": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "pre_sus_time": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "hold": false,
      "priority": {
        "set": true,
        "infinite": false,
        "number": 1
      },
      "profile": [
        "NOT_SET"
      ],
      "qos": "normal",
      "reboot": false,
      "required_nodes": "",
      "minimum_switches": 0,
      "requeue": true,
      "resize_time": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "restart_cnt": 0,
      "resv_name": "",
      "scheduled_nodes": "",
      "selinux_context": "",
      "shared": [
      ],
      "exclusive": [
      ],
      "oversubscribe": true,
      "show_flags": [
        "ALL",
        "DETAIL",
        "LOCAL"
      ],
      "sockets_per_board": 0,
      "sockets_per_node": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "start_time": {
        "set": true,
        "infinite": false,
        "number": 1746693312
      },
      "state_description": "",
      "state_reason": "None",
      "standard_error": "\/home\/user\/test\/results\/myjob-308.stderr",
      "standard_input": "\/dev\/null",
      "standard_output": "\/home\/user\/test\/results\/myjob-308.stdout",
      "submit_time": {
        "set": true,
        "infinite": false,
        "number": 1746693311
      },
      "suspend_time": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "system_comment": "",
      "time_limit": {
        "set": false,
        "infinite": true,
        "number": 0
      },
      "time_minimum": {
        "set": true,
        "infinite": false,
        "number": 0
      },
      "threads_per_core": {
        "set": false,
        "infinite": false,
        "number": 0
      },
      "tres_bind": "",
      "tres_freq": "",
      "tres_per_job": "",
      "tres_per_node": "",
      "tres_per_socket": "",
      "tres_per_task": "",
      "tres_req_str": "cpu=1,mem=15737M,node=1,billing=1",
      "tres_alloc_str": "cpu=1,mem=15737M,node=1,billing=1",
      "user_id": 1000,
      "user_name": "qc-rest",
      "maximum_switch_wait_time": 0,
      "wckey": "",
      "current_working_directory": "\/home\/user\/test"
    }
  ],
  "last_backfill": {
    "set": true,
    "infinite": false,
    "number": 1746670919
  },
  "last_update": {
    "set": true,
    "infinite": false,
    "number": 1746693325
  },
  "meta": {
    "plugin": {
      "type": "openapi\/slurmctld",
      "name": "Slurm OpenAPI slurmctld",
      "data_parser": "data_parser\/v0.0.41",
      "accounting_storage": "accounting_storage\/slurmdbd"
    },
    "client": {
      "source": "[hostname]:9876",
      "user": "root",
      "group": "root"
    },
    "command": [
    ],
    "slurm": {
      "version": {
        "major": "24",
        "micro": "3",
        "minor": "05"
      },
      "release": "24.05.3",
      "cluster": "cluster"
    }
  },
  "errors": [
  ],
  "warnings": [
  ]
}
    )";
    const std::string expectedMessage = "Job not found: 333";

    rexapis::SlurmGetJobStatusResponseParser parser{jobId};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::JsonParseError>(result)) {
        const rexapis::JsonParseError& err = std::get<rexapis::JsonParseError>(result);
        ASSERT_EQ(expectedMessage, err.GetMessage());
    } else {
        FAIL();
    }
}

TEST(SlurmGetJobStatusResponseParser, ParseInvalidJson) {
    const std::string jobId = "333";
    const std::string jsonStr = R"(
 {
  "jobs": [
    {
      "account": "",
      "accrue_time": {
        "set": true,
        "major": "24",
    )";
    const std::string expectedMessage = "* Line 9, Column 4 Syntax error: Malformed object literal";

    rexapis::SlurmGetJobStatusResponseParser parser{jobId};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::JsonParseError>(result)) {
        const rexapis::JsonParseError& err = std::get<rexapis::JsonParseError>(result);
        ASSERT_EQ(expectedMessage, err.GetMessage());
    } else {
        FAIL();
    }
}

TEST(SlurmGetJobStatusResponseParser, ParseEmptyJson) {
    const std::string jobId = "333";
    const std::string jsonStr = "";
    const std::string expectedMessage = "* Line 1, Column 1 Syntax error: Malformed token";

    rexapis::SlurmGetJobStatusResponseParser parser{jobId};
    auto result = parser.Parse(jsonStr);
    if (std::holds_alternative<rexapis::JsonParseError>(result)) {
        const rexapis::JsonParseError& err = std::get<rexapis::JsonParseError>(result);
        ASSERT_EQ(expectedMessage, err.GetMessage());
    } else {
        FAIL();
    }
}

}

