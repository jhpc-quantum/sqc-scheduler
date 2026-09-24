OPENQASM 3;
include "stdgates.inc";

qreg q[1];
creg c[1];

measure q[0] -> c[0];
