#/usr/bin/env bash

set +e
echo change_id: ${CHANGE_ID}
echo branch_name: ${BRANCH_NAME}
echo change_url: ${CHANGE_URL}
echo change_title: ${CHANGE_TITLE}
echo change_target: ${CHANGE_TARGET}
echo git_url: ${GIT_URL}

source tools/jenkins/slurm-constraint-${configuration_name}.sh
echo github_token: ${GITHUB_TOKEN}
rm -rf build
rm -f jenkins-hpx*
#sbatch \
#    --job-name="jenkins-hpx-${configuration_name}" \
#    --nodes="1" \
#    --constraint="${configuration_slurm_constraint}" \
#    --partition="cscsci" \
#    --time="02:00:00" \
#    --output="jenkins-hpx-${configuration_name}.out" \
#    --error="jenkins-hpx-${configuration_name}.err" \
#    --wait tools/jenkins/batch.sh
#
#echo "= stdout =================================================="
#cat jenkins-hpx-${configuration_name}.out
#
#echo "= stderr =================================================="
#cat jenkins-hpx-${configuration_name}.err

# Set GitHub status with CDash url

# Get build status
#if [[ $(cat jenkins-hpx-${configuration_name}-ctest-status.txt) ]]; then
#    github_commit_status="success"
#else
#    github_commit_status="failure"
#fi
github_commit_status="failure"

# Extract just the organization and repo names "org/repo"
github_commit_repo="$(echo $GIT_URL | sed 's/.*\\/\\([^\\/]*\\/[^\\/]*\\).git/\\1/')"

# Get the CDash dashboard build id
#cdash_build_id="$(cat jenkins-hpx-${configuration_name}-cdash-build-id.txt)"
cdash_build_id="123"

# Extract actual token from GITHUB_TOKEN (in the form "username:token")
github_token=$(echo ${GITHUB_TOKEN} | cut -f2 -d':')
./tools/jenkins/set_github_status.sh \
    "${github_token}" \
    "${github_commit_repo}" \
    "${GIT_COMMIT}" \
    "${github_commit_status}" \
    "${configuration_name}" \
    "${cdash_build_id}"

exit $(cat jenkins-hpx-${configuration_name}-ctest-status.txt)
