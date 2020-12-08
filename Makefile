# Configuration
.PHONY: build-app-image 
.DEFAULT_GOAL := help

build-app-image-master: ## Build Asgard app image from master
	$(info Building Asgard app image from master)
	docker build -f docker/asgard-app/Dockerfile -t navitia/asgard-app:${tag} . --no-cache

build-app-image-release: ## Build Asgard app image from release
	$(info Building Asgard app image from release)
	docker build -f docker/asgard-app/Dockerfile -t navitia/asgard-app:${tag} . --no-cache

dockerhub-login: ## Login Docker hub, DOCKERHUB_USER, DOCKERHUB_PWD, must be provided
	$(info Login Dockerhub)
	echo ${DOCKERHUB_PWD} | docker login --username ${DOCKERHUB_USER} --password-stdin

get-app-master-tag: ## Get master tag
	@echo "master"

get-app-release-tag: ## Get release version tag
	@[ -z "${tag}" ] && (git describe --tags --abbrev=0 && exit 0) || echo ${tag}

push-app-image: ## Push app-image to dockerhub
	$(info Push data-image to Dockerhub)
	docker push navitia/asgard-app:${tag}

wipe-useless-images: ## Remove all useless images
	$(info Remove useless images)
	@dangling_images=`docker images --filter "dangling=true" -q --no-trunc`;
	@[ "${dangling_images}" ] && docker rmi -f $(dangling_images) || ( echo "No Dangling Images")
	@asgard_images=`docker images "navitia/asgard-*" -q`;
	@[ "${asgard_images}" ] && docker rmi -f $(asgard_images) || (echo "No Asgard Images")

##
## Miscellaneous
##
help: ## Print this help message
	@grep -E '^[a-zA-Z_-]+:.*## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

