pipeline {
    agent any
    stages {
        stage('Build App Image') {
	    when {
		expression {
		    env.BRANCH_NAME == "add_jenkinsfile_create_asgard_app_from_master"
	    	}
	    }
            steps {
		# docker build -f docker/asgard-app/Dockerfile -t navitia/asgard-app:master . --no-cache
		echo 'test 1'
            }
        }
        stage('Login') {
            steps {
		# withCredentials([[$class: 'UsernamePasswordMultiBinding', credentialsId: 'jenkins-core-dockerhub', usernameVariable: 'USERNAME', passwordVariable: 'PASSWORD']] {
		# docker login -u=navitia -p=${PASSWORD}
		echo 'test 2'
		# }
            }
        }
	stage('Push Asgard App Image') {
	    steps {
		# docker push navitia/asgard-app:master
		echo 'test 3'
	    }
	}
	stage('Delete') {
	    steps {
		# delete all dangling and 'navitia/asgard-*' images
		# docker rmi -f $(docker images --filter "dangling=true" -q --no-trunc) || true
		# docker rmi -f $(docker images "navitia/asgard-*" -q)
		echo 'test 3'
	    }
	}
    }
    post {
        always {
            deleteDir()
        }
    }
}

