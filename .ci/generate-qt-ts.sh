#!/bin/sh -ex

mkdir -p ../translations

LUPDATE_PATH=$(find /usr -name lupdate -type f 2>/dev/null | head -n 1)
if [ -z "$LUPDATE_PATH" ]; then
  echo "Error: lupdate not found!"
  exit 1
else
  echo "lupdate found at: $LUPDATE_PATH"
  $LUPDATE_PATH -recursive . -ts ../translations/rpcs3_template.ts
  sed -i 's|filename="\.\./|filename="./|g' ../translations/rpcs3_template.ts
fi