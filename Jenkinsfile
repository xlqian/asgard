pipeline {
    agent any
    stages {
        stage('stage 1') {
            steps {
                echo "toto 1"
            }
        }
        stage('stage 1') {
            steps {
                echo "toto 2"
            }
        }
        stage('stage 1') {
            steps {
                echo "toto 3"
            }
        }
    }
    post {
        always {
            deleteDir()
        }
    }
}

