REGISTRY=docker.io
REPO=isilincoln
IMAGE=orchestrator-sabres-cbs
TAG=latest
BUILD_ARGS=--no-cache

sudo docker build ${BUILD_ARGS} -f Dockerfile -t ${REGISTRY}/${REPO}/${IMAGE}:${TAG} .
sudo docker push ${REGISTRY}/${REPO}/${IMAGE}:${TAG}
