#!/bin/bash


export DOCKER_BUILDER_TMP_IMAGE="gkrellflynn_builder_image";
export DOCKER_BUILDER_TMP_CONTAINER="gkrellflynn_builder";
export DOCKER_BUILDER_TARGET_FILE="/app/gkrellflynn.so";


./docker_builder.sh;
if [ "${?}" != "0" ];
then
	exit 1;
fi


echo "ok, file ${DOCKER_BUILDER_TARGET_FILE} is ready";


exit 0;
