#!/bin/bash
#---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
# 1.0.0
# Alexey Potehin <gnuplanet@gmail.com>, http://www.gnuplanet.ru/doc/cv
#---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
# general function
function main()
{
	if [ "${DOCKER_BUILDER_TMP_IMAGE}" == "" ];
	then
		echo "ERROR: you must set DOCKER_BUILDER_TMP_IMAGE";
		return 1;
	fi

	if [ "${DOCKER_BUILDER_TMP_CONTAINER}" == "" ];
	then
		echo "ERROR: you must set DOCKER_BUILDER_TMP_CONTAINER";
		return 1;
	fi

	if [ "${DOCKER_BUILDER_TARGET_FILE}" == "" ];
	then
		echo "ERROR: you must set DOCKER_BUILDER_TARGET_FILE";
		return 1;
	fi

	which docker &> /dev/null < /dev/null;
	if [ "${?}" != "0" ];
	then
		echo "ERROR: docker is not found";
		return 1;
	fi

	docker build --no-cache --tag "${DOCKER_BUILDER_TMP_IMAGE}" ./;
	if [ "${?}" != "0" ];
	then
		echo "ERROR: docker build";
		return 1;
	fi

	docker create --name "${DOCKER_BUILDER_TMP_CONTAINER}" "${DOCKER_BUILDER_TMP_IMAGE}";
	if [ "${?}" != "0" ];
	then
		echo "ERROR: docker create";
		return 1;
	fi

	docker cp "${DOCKER_BUILDER_TMP_CONTAINER}":"${DOCKER_BUILDER_TARGET_FILE}" ./;
	if [ "${?}" != "0" ];
	then
		echo "ERROR: docker cp";
		return 1;
	fi

	docker rm -f "${DOCKER_BUILDER_TMP_CONTAINER}";
	if [ "${?}" != "0" ];
	then
		echo "ERROR: docker rm";
		return 1;
	fi

	docker rmi "${DOCKER_BUILDER_TMP_IMAGE}";
	if [ "${?}" != "0" ];
	then
		echo "ERROR: docker rmi";
		return 1;
	fi


	return 0;
}
#---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
main "${@}";

exit "${?}";
#---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------#
