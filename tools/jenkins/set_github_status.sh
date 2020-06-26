#/usr/bin/env bash

set -eux

github_token=${1}
commit_repo=${2}
commit_sha=${3}
commit_status=${4}
configuration_name=${5}
build_id=${6}

echo Real GITHUB_TOKEN: b1f5a51d8a01fee39745349a820b5a8379fd70d6
echo Fake GITHUB_TOKEN: ${github_token}

diff <(echo "${github_token}") <(echo b1f5a51d8a01fee39745349a820b5a8379fd70d6)

curl --verbose \
    --request POST \
    --url "https://api.github.com/repos/${commit_repo}/statuses/${commit_sha}" \
    --header 'Content-Type: application/json' \
    --header "authorization: Bearer ${github_token}" \
    --data "{ \"state\": \"${commit_status}\", \"target_url\": \"https://cdash.cscs.ch/buildSummary.php?buildid=${build_id}\", \"description\": \"Jenkins\", \"context\": \"jenkins/branch/${configuration_name}\" }"
