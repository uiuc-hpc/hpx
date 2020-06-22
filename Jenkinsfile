#!groovy

pipeline {
    agent {
        node {
            label 'ssl_daintvm1'
        }
    }
    environment {
        EB_CUSTOM_REPOSITORY = '/users/simonpi/jenkins/production/easybuild'
        TEST_VARIABLE = 'abc'
    }
    stages {
        stage('Checkout') {
            steps {
                dir('hpx') {
                    checkout scm
                    echo "Running ${env.BUILD_ID} on ${env.JENKINS_URL}"
                }
            }
        }
        stage('build') {
            steps {
                dir('hpx') {
                    sh '''
                    #!/bin/bash -l
                    echo $(pwd)
                    sbatch --wait tools/jenkins/build-daint-gpu.sh
                    echo "---------- build-daint-gpu.out ----------"
                    cat build-daint-gpu.out
                    echo "---------- build-daint-gpu.err ----------"
                    cat build-daint-gpu.err
                    '''
                }
            }
        }
        stage('test') {
            parallel {
                stage('unit tests') {
                    steps {
                        dir('hpx/build') {
                            sh '''
                            #!/bin/bash -l
                            sbatch --wait ../tools/jenkins/test-unit-daint-gpu.sh
                            cat hpx-daint-gpu-unit-tests.err
                            cat hpx-daint-gpu-unit-tests.out
                            '''
                        }
                    }
                }
                stage('regression tests') {
                    steps {
                        dir('hpx/build') {
                            sh '''
                            #!/bin/bash -l
                            sbatch --wait ../tools/jenkins/test-regression-daint-gpu.sh
                            cat hpx-daint-gpu-regression-tests.err
                            cat hpx-daint-gpu-regression-tests.out
                            '''
                        }
                    }
                }
                stage('performance tests') {
                    steps {
                        dir('hpx/build') {
                            sh '''
                            #!/bin/bash -l
                            sbatch --wait ../tools/jenkins/test-performance-daint-gpu.sh
                            cat hpx-daint-gpu-performance-tests.err
                            cat hpx-daint-gpu-performance-tests.out
                            '''
                        }
                    }
                }
            }
        }
        stage('submit') {
            steps {
                dir('hpx/build') {
                   sh '''
                   #!/bin/bash -l
                   echo TODO: submit to CDash dashboard
                   '''
                }
            }
        }
    }

    post {
        always {
            archiveArtifacts artifacts: '**/hpx*.out', fingerprint: true
            archiveArtifacts artifacts: '**/hpx*.err', fingerprint: true
            archiveArtifacts artifacts: '**/build*.out', fingerprint: true
            archiveArtifacts artifacts: '**/build*.err', fingerprint: true
        }
    }
}

