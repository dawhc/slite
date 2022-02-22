FROM alpine:latest

RUN apk add --update gcc make cmake

COPY . /root/slite

RUN bash ./setup.sh

ENTRYPOINT ["build/slite"]
