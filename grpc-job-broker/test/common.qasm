OPENQASM 3;
include "stdgates.inc";

qreg q[4];
creg c[4];
h q[0];
h q[1];

ccx q[0],q[1],q[2];
cx q[0],q[3];
cx q[1],q[3];

measure q[3] -> c[0];
measure q[2] -> c[1];
measure q[1] -> c[2];
measure q[0] -> c[3];
