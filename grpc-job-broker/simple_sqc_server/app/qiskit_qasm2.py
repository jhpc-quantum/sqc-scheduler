import json, os
from qiskit import qasm2
from qiskit_aer import AerSimulator

TIMEOUT_SECONDS = 30

def save_error_file(error_file: str, error_message: str) -> None:
    try:
        with open(error_file, mode="w", encoding="utf-8") as f:
            f.write(error_message)
    except Exception as e:
        pass

def simulate_quantum_circuit(priority: int,
                             qprogram: str,
                             circuit_fmt: int,
                             shots: int,
                             qc_type: int,
                             transpiler: int,
                             remark: str,
                             result_file: str,
                             error_file: str) -> tuple[bool, str]:
    tmp_result_file = result_file + ".tmp"

    try:
        qiskit_circuit = qasm2.loads(qprogram)
        job = AerSimulator().run(circuits=qiskit_circuit, shots=shots, memory=True)
        dict_result = job.result(timeout=TIMEOUT_SECONDS).to_dict()

        with open(tmp_result_file, mode="w", encoding="utf-8") as f:
            json.dump(dict_result, f, ensure_ascii=True)
        os.rename(tmp_result_file, result_file)
        return True, ""
    except Exception as e:
        os.remove(tmp_result_file)
        save_error_file(error_file, f"error: {repr(e)}")
        return False, ""
