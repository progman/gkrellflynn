#!/bin/bash

which docker &> /dev/null < /dev/null;
if [ "${?}" != "0" ];
then
	echo "ERROR: docker is not found";
	exit 1;
fi

docker build --no-cache --tag gkrellflynn_builder_image ./;
if [ "${?}" != "0" ];
then
	echo "ERROR: docker build";
	exit 1;
fi

docker create --name gkrellflynn_builder gkrellflynn_builder_image;
if [ "${?}" != "0" ];
then
	echo "ERROR: docker create";
	exit 1;
fi

docker cp gkrellflynn_builder:/app/gkrellflynn.so ./;
if [ "${?}" != "0" ];
then
	echo "ERROR: docker cp";
	exit 1;
fi

docker rm -f gkrellflynn_builder;
if [ "${?}" != "0" ];
then
	echo "ERROR: docker rm";
	exit 1;
fi

docker rmi gkrellflynn_builder_image;
if [ "${?}" != "0" ];
then
	echo "ERROR: docker rmi";
	exit 1;
fi

echo "gkrellflynn.so is ready";

exit 0;
