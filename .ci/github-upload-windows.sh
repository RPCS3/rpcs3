#!/bin/sh -ex

ARTIFACT_DIR="$BUILD_ARTIFACTSTAGINGDIRECTORY"
generate_post_data()
{
    body=$(cat GitHubReleaseMessage.txt)
    cat <<EOF
    {
    "tag_name": "build-${BUILD_SOURCEVERSION}",
    "target_commitish": "7d09e3be30805911226241afbb14f8cdc2eb054e",
    "name": "${AVVER}",
    "body": "$body",
    "draft": false,
    "prerelease": false
    }
EOF
}

repo_full_name="RPCS3/rpcs3-binaries-win"

curl -s \
    -H "Authorization: token ${RPCS3_TOKEN}" \
    -H "Accept: application/vnd.github.v3+json" \
    --data "$(generate_post_data)" "https://api.github.com/repos/$repo_full_name/releases" >> release.json

id=$(grep '"id"' release.json | cut -d ' ' -f4 | head -n1)
id=${id%?}
echo ${id:?}

upload_file()
{
    curl -s \
        -H "Authorization: token ${RPCS3_TOKEN}" \
        -H "Accept: application/vnd.github.v3+json" \
        -H "Content-Type: application/octet-stream" \
        --data-binary @"$2"/"$3" \
        "https://uploads.github.com/repos/$repo_full_name/releases/$1/assets?name=$3"
}

for file in "$ARTIFACT_DIR"/*; do
    name=$(basename "$file")
    upload_file "$id" "$ARTIFACT_DIR" "$name"
done
