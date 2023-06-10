FROM debian:stable-20230522-slim

WORKDIR /app/
COPY . /app/

RUN apt-get update; \
    echo y | apt-get install libglib2.0-dev; \
    echo y | apt-get install libgtk2.0-dev; \
    echo y | apt-get install gkrellm; \
    echo y | apt-get install gcc; \
    echo y | apt-get install make; \
    ln -sf /usr/include/glib-2.0/glib.h /usr/include/glib.h; \
    ln -sf /usr/include/glib-2.0/glib /usr/include/glib; \
    ln -sf /usr/lib/x86_64-linux-gnu/glib-2.0/include/glibconfig.h /usr/include/glibconfig.h; \
    make gkrellm2;
