#! /bin/sh

#
# Create ES256 private and public key files in PEM format.
#
create_es256_key_files() {
  (
    PUBLIC_KEY_FILE=$1
    PRIVATE_KEY_FILE=$2
    rm -f "$PUBLIC_KEY_FILE" "$PRIVATE_KEY_FILE"
    openssl ecparam -name secp256k1 -genkey -noout -out "$PRIVATE_KEY_FILE" \
      && openssl ec -in "$PRIVATE_KEY_FILE" -pubout > "$PUBLIC_KEY_FILE"
  )
}

#
# Append a claim (a pair of a key and a value) to the list.
#
_append_claim_if() {
  (
    LIST=$1
    KEY=$2
    VALUE=$3

    if [ "$VALUE" != '' ]; then
      if [ "$LIST" = '' ]; then
        printf '"%s":%s' "$KEY" "$VALUE"
      else
        printf '%s,"%s":%s' "$LIST" "$KEY" "$VALUE"
      fi
    else
      printf '%s' "$LIST"
    fi
  )
}

#
# Create a JWT.
#
create_jwt() {
  (
    PRIVATE_KEY_FILE=$1
    TYP=$2
    ALG=$3
    IAT=$4
    EXP=$5
    ISS=$6
    SUB=$7

    HEADER_CLAIMS=
    HEADER_CLAIMS=$(_append_claim_if "$HEADER_CLAIMS" "typ" "$TYP")
    HEADER_CLAIMS=$(_append_claim_if "$HEADER_CLAIMS" "alg" "$ALG")

    DATA_CLAIMS=
    DATA_CLAIMS=$(_append_claim_if "$DATA_CLAIMS" "iat" "$IAT")
    DATA_CLAIMS=$(_append_claim_if "$DATA_CLAIMS" "exp" "$EXP")
    DATA_CLAIMS=$(_append_claim_if "$DATA_CLAIMS" "iss" "$ISS")
    DATA_CLAIMS=$(_append_claim_if "$DATA_CLAIMS" "sub" "$SUB")

    BASE64_HEADER=$(printf '{%s}' "$HEADER_CLAIMS" \
                      | base64 --wrap=0 \
                      | tr -d '\r\n=' \
                      | tr '+/' '-_')
    BASE64_DATA=$(printf '{%s}' "$DATA_CLAIMS" \
                    | base64 --wrap=0 \
                    | tr -d '\r\n=' \
                    | tr '+/' '-_')

    BASE64_MESSAGE=$(printf '%s.%s' "$BASE64_HEADER" "$BASE64_DATA")
    BASE64_SIGNATURE=$(printf '%s' "$BASE64_MESSAGE" \
                         | openssl dgst -sha256 -sign "$PRIVATE_KEY_FILE" \
                         | openssl asn1parse -inform DER \
                         | perl -n -e '/INTEGER\s+:([0-9A-Z]*)$/ && print $1' \
                         | xxd -p -r \
                         | base64 --wrap=0 \
                         | tr -d '\r\n=' \
                         | tr '+/' '-_')

    printf '%s.%s' "$BASE64_MESSAGE" "$BASE64_SIGNATURE"
  )
}

#
# Get header part of a JWT.
#
get_jwt_header() {
  awk -F. '{print $1}'
}

#
# Get data part of a JWT.
#
get_jwt_data() {
  awk -F. '{print $2}'
}

#
# Get signature part of a JWT.
#
get_jwt_signature() {
  awk -F. '{print $3}'
}

#
# Get an epoch time of the given date time string.
#
unquote() {
  printf '%s' "$1" | sed -e 's|^"||' -e 's|"$||'
}

#
# Get the current epoch time.
#
epoch_now() {
  date +%s
}

#
# Get an epoch time of the given date time string.
#
epoch_with_datetime() {
  date +%s "$1"
}
