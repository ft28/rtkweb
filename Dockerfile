FROM ubuntu:16.04

ARG user_id
ARG user_name
ARG group_id
ARG group_name

RUN apt-get update && \
    apt-get install -y gcc build-essential git cmake libssl-dev libssl1.0.0 libuv1-dev libsqlite3-dev && \
    apt-get install -y vim tmux curl net-tools && \
    mkdir /var/log/lwsws && \
    cd /opt && \
    git clone https://github.com/warmcat/libwebsockets.git && \
    cd libwebsockets && \
    cmake -DLWS_WITH_LWSWS=1 -DLWS_WITH_GENERIC_SESSIONS=1 . && \
    make && \
    make install && \
    ldconfig

RUN  mkdir /opt/rtkweb  && \
     addgroup --gid ${group_id} ${group_name} && \
     adduser  --gid ${group_id} --uid ${user_id} ${user_name} && \
     chown -R ${user_name}:${gropu_name} /opt/rtkweb

USER ${user_name}
WORKDIR /opt/rtkweb

CMD ["/bin/bash"]
