FROM alpine:3.8
MAINTAINER br@rexos.org

RUN apk --update add make g++ minizip
RUN apk add cmake

COPY entrypoint.sh /

RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
