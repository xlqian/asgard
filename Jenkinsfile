pipeline {
    agent any
    stages {
        stage('stage 1') {
            steps {
		# docker build -f docker/asgard-app/Dockerfile -t navitia/asgard-app:master . --no-cache
		echo 'test 1'
            }
        }
        stage('stage 2') {
            steps {
		# docker login -u=navitia -p=${password}
		# docker push navitia/asgard-app:master
		echo 'test 2'
            }
        }
	stage('stage 3') {
	    steps {
		# delete all dangling and 'navitia/asgard-*' images
		# docker rmi -f $(docker images --filter "dangling=true" -q --no-trunc) || true
		# docker rmi -f $(docker images "navitia/asgard-*" -q)
		echo 'test 3'
    }
    post {
        always {
            deleteDir()
        }
    }
}

