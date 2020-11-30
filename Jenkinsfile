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
		echo 'test 1'
            }
        }
        stage('Login') {
            steps {
		echo 'test 2'
            }
        }
	stage('Push Asgard App Image') {
	    steps {
		echo 'test 3'
	    }
	}
	stage('Delete') {
	    steps {
		echo 'test 4'
	    }
	}
    }
    post {
        always {
            deleteDir()
        }
    }
}

