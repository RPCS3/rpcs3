#!/bin/sh -ex

ARTIFACT_DIR="$BUILD_ARTIFACTSTAGINGDIRECTORY"
generate_post_data()
{
    body=$(cat GitHubReleaseMessage.txt)
    cat <<EOF
    {
    "tag_name": "build-${BUILD_SOURCEVERSION}",
    "target_commitish": "${UPLOAD_COMMIT_HASH}",
    "name": "${AVVER}",
    "body": "$body",
    "draft": false,
    "prerelease": false
    }
EOF
}

curl -fsS \
    -H "Authorization: token ${RPCS3_TOKEN}" \
    -H "Accept: application/vnd.github.v3+json" \
    --data "$(generate_post_data)" "https://api.github.com/repos/$UPLOAD_REPO_FULL_NAME/releases" >> release.json

cat release.json
id=$(grep '"id"' release.json | cut -d ':' -f2 | head -n1 | awk '{$1=$1;print}')
id=${id%?}
echo "${id:?}"

upload_file()
{
    curl -fsS \
        -H "Authorization: token ${RPCS3_TOKEN}" \
        -H "Accept: application/vnd.github.v3+json" \
        -H "Content-Type: application/octet-stream" \
        --data-binary @"$2"/"$3" \
        "https://uploads.github.com/repos/$UPLOAD_REPO_FULL_NAME/releases/$1/assets?name=$3"
}

for file in "$ARTIFACT_DIR"/*; do
    name=$(basename "$file")
    upload_file "$id" "$ARTIFACT_DIR" "$name"
done
