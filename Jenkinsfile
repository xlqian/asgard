pipeline {
    agent any
    parameters { 
        // Parameters for Data Image
        booleanParam(name: 'BUILD_IMAGE_ONLY', defaultValue: false, description: '')
        string(name: 'PBF_URL', defaultValue: 'https://download.geofabrik.de/europe/france-latest.osm.pbf', description: 'osm pbf url')
        string(name: 'DATA_IMAGE_TAG', defaultValue : 'france', description: 'data image tag')
        string(name: 'ELEVATION_BBOX', defaultValue : '-26 38 46 80', description: 'elevation bounding box, default value is for france')
    }
    stages {
        stage('Build Data Image') {
            when {
                expression {
                    env.BRANCH_NAME == "add_jenkinsfile_create_asgard_data"                
                }
            }
            steps {
                sh "ls"
                sh "make build-data-image TAG=${params.DATA_IMAGE_TAG} PBF_URL=${params.PBF_URL} BBOX=${params.ELEVATION_BBOX}" 
            }
        }
        stage('Log in') {
            steps {
                withCredentials([[$class: 'UsernamePasswordMultiBinding', credentialsId:'jenkins-core-dockerhub', usernameVariable: 'USERNAME', passwordVariable: 'PASSWORD']]) {
                    sh "make dockerhub-login DOCKERHUB_USER=${USERNAME} DOCKERHUB_PWD=${PASSWORD}"
                }
            }
        }
        stage('Push Asgard Data Image') {
            steps {
                echo "toto 4"
            }
        }
    }
    post {
        always {
            deleteDir()
        }
    }
}

