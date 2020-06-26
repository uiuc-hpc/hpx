#!/bin/bash -l

set -eux

github_token=${1}
commit_repo=${2}
commit_sha=${3}
commit_status=${4}
configuration_name=${5}
build_id=${6}

for (( i=0; i<${#github_token}; i++ )); do echo "${github_token:$i:1}"; done

curl --verbose \
    --request POST \
    --url "https://api.github.com/repos/${commit_repo}/statuses/${commit_sha}" \
    --header 'Content-Type: application/json' \
    --header "authorization: Bearer ${github_token}" \
    --data "{ \"state\": \"${commit_status}\", \"target_url\": \"https://cdash.cscs.ch/buildSummary.php?buildid=${build_id}\", \"description\": \"Jenkins\", \"context\": \"jenkins/branch/${configuration_name}\" }"
