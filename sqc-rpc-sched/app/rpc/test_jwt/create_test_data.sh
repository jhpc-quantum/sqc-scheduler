#! /bin/sh

srcdir=${srcdir:-"."}

. "${srcdir}/common.sh"

VALID_TYP='"JWT"'
VALID_ALG='"ES256"'
VALID_IAT=$(expr $(epoch_now) - 2592000)
VALID_EXP=$(expr $VALID_IAT + 5184000)
VALID_ISS='"https://www.example.com/auth"'
VALID_SUB='"01234567-89ab-cdef-0123-456789abcdef"'

EXP_RESULT_PREFIX=exp_result.txt
PUBLIC_KEY_PREFIX=jwt_pub.key
PRIVATE_KEY_PREFIX=jwt_priv.key
JWT_PREFIX=jwt.token
ISS_PREFIX=jwt_iss.txt
SUB_PREFIX=jwt_sub.txt

#
# Create file for a test case.
#
create_test_case_files() {
  (
    TEST_NAME=$1
    EXP_RESULT=$2
    TYP=$3
    ALG=$4
    IAT=$5
    EXP=$6
    ISS=$7
    SUB=$8

    EXP_RESULT_FILE=__${TEST_NAME}/${EXP_RESULT_PREFIX}
    PUBLIC_KEY_FILE=__${TEST_NAME}/${PUBLIC_KEY_PREFIX}
    PRIVATE_KEY_FILE=__${TEST_NAME}/${PRIVATE_KEY_PREFIX}
    JWT_FILE=__${TEST_NAME}/${JWT_PREFIX}
    ISS_FILE=__${TEST_NAME}/${ISS_PREFIX}
    SUB_FILE=__${TEST_NAME}/${SUB_PREFIX}

    trap "rm -r -f '__${TEST_NAME}'; exit 1" 1 2 3 15
    mkdir -p "__${TEST_NAME}" \
      && printf '%s' "$EXP_RESULT" > "$EXP_RESULT_FILE" \
      && printf '%s' $(unquote "$ISS") > "$ISS_FILE" \
      && printf '%s' $(unquote "$SUB") > "$SUB_FILE" \
      && create_es256_key_files "$PUBLIC_KEY_FILE" "$PRIVATE_KEY_FILE" \
      && create_jwt "$PRIVATE_KEY_FILE" "$TYP" "$ALG" "$IAT" "$EXP" "$ISS" "$SUB" > "$JWT_FILE"
    [ $? -eq 0 ] || { echo "failed to create test data '$TEST_NAME'" >&2; exit 1; }
  )
}

#
# test case: valid
#
TEST_NAME=valid
create_test_case_files "$TEST_NAME" \
                       pass \
                       "$VALID_TYP" \
                       "$VALID_ALG" \
                       "$VALID_IAT" \
                       "$VALID_EXP" \
                       "$VALID_ISS" \
                       "$VALID_SUB"

#
# test case: missing alg
#
TEST_NAME=missing_alg
create_test_case_files "$TEST_NAME" \
                       fail \
                       "$VALID_TYP" \
                       '' \
                       "$VALID_IAT" \
                       "$VALID_EXP" \
                       "$VALID_ISS" \
                       "$VALID_SUB"

#
# test case: invalid alg
#
TEST_NAME=invalid_alg
create_test_case_files "$TEST_NAME" \
                       fail \
                       "$VALID_TYP" \
                       '"HS256"' \
                       "$VALID_IAT" \
                       "$VALID_EXP" \
                       "$VALID_ISS" \
                       "$VALID_SUB"

#
# test case: missing iat
#
TEST_NAME=missing_iat
create_test_case_files "$TEST_NAME" \
                       pass \
                       "$VALID_TYP" \
                       "$VALID_ALG" \
                       '' \
                       "$VALID_EXP" \
                       "$VALID_ISS" \
                       "$VALID_SUB"

#
# test case: future iat
#
TEST_NAME=future_iat
create_test_case_files "$TEST_NAME" \
                       fail \
                       "$VALID_TYP" \
                       "$VALID_ALG" \
                       $(expr $(epoch_now) + 1296000) \
                       $(expr $(epoch_now) + 1296000 + 1296000) \
                       "$VALID_ISS" \
                       "$VALID_SUB"

#
# test case: missing exp
#
TEST_NAME=missing_exp
create_test_case_files "$TEST_NAME" \
                       fail \
                       "$VALID_TYP" \
                       "$VALID_ALG" \
                       "$VALID_IAT" \
                       '' \
                       "$VALID_ISS" \
                       "$VALID_SUB"

#
# test case: passed exp
#
TEST_NAME=passed_exp
create_test_case_files "$TEST_NAME" \
                       fail \
                       "$VALID_TYP" \
                       "$VALID_ALG" \
                       $(expr $(epoch_now) - 1296000 - 1296000) \
                       $(expr $(epoch_now) - 1296000) \
                       "$VALID_ISS" \
                       "$VALID_SUB"

#
# test case: missing iss
#
TEST_NAME=missing_iss
create_test_case_files "$TEST_NAME" \
                       fail \
                       "$VALID_TYP" \
                       "$VALID_ALG" \
                       "$VALID_IAT" \
                       "$VALID_EXP" \
                       '' \
                       "$VALID_SUB"

printf '%s' $(unquote "$VALID_ISS") > "__${TEST_NAME}/${ISS_PREFIX}"

#
# test case: mismatched iss
#
TEST_NAME=mismatched_iss
create_test_case_files "$TEST_NAME" \
                       fail \
                       "$VALID_TYP" \
                       "$VALID_ALG" \
                       "$VALID_IAT" \
                       "$VALID_EXP" \
                       '"dummy"' \
                       "$VALID_SUB"

printf '%s' $(unquote "$VALID_ISS") > "__${TEST_NAME}/${ISS_PREFIX}"

#
# test case: missing sub
#
TEST_NAME=missing_sub
create_test_case_files "$TEST_NAME" \
                       fail \
                       "$VALID_TYP" \
                       "$VALID_ALG" \
                       "$VALID_IAT" \
                       "$VALID_EXP" \
                       "$VALID_ISS" \
                       ''

printf '%s' $(unquote "$VALID_SUB") > "__${TEST_NAME}/${SUB_PREFIX}"
