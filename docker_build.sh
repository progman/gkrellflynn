#!/bin/bash

DOCKER_IMAGE="gkrellflynn_builder_image";
DOCKER_CONTAINER="gkrellflynn_builder";
TARGET_FILE="gkrellflynn.so";

which docker &> /dev/null < /dev/null;
if [ "${?}" != "0" ];
then
	echo "ERROR: docker is not found";
	exit 1;
fi

docker build --no-cache --tag "${DOCKER_IMAGE}" ./;
if [ "${?}" != "0" ];
then
	echo "ERROR: docker build";
	exit 1;
fi

docker create --name "${DOCKER_CONTAINER}" "${DOCKER_IMAGE}";
if [ "${?}" != "0" ];
then
	echo "ERROR: docker create";
	exit 1;
fi

docker cp "${DOCKER_CONTAINER}":/app/"${TARGET_FILE}" ./;
if [ "${?}" != "0" ];
then
	echo "ERROR: docker cp";
	exit 1;
fi

docker rm -f "${DOCKER_CONTAINER}";
if [ "${?}" != "0" ];
then
	echo "ERROR: docker rm";
	exit 1;
fi

docker rmi "${DOCKER_IMAGE}";
if [ "${?}" != "0" ];
then
	echo "ERROR: docker rmi";
	exit 1;
fi

echo "Done";

exit 0;
