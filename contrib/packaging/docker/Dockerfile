FROM library/debian:9 as build
RUN apt-get update \
  && apt-get install -y --no-install-recommends build-essential git \
  meson ninja-build gcc cpp cmake libssl-dev libmediainfo-dev \
  openjdk-8-jre-headless perl \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8
RUN mkdir /libmpdclient-dist
RUN git clone https://github.com/MusicPlayerDaemon/libmpdclient.git
WORKDIR /libmpdclient
RUN meson . output
RUN ninja -C output
RUN ninja -C output install
RUN mesonconf output -Dprefix=/libmpdclient-dist
RUN ninja -C output
RUN ninja -C output install
WORKDIR /
RUN tar -czvf /libmpdclient-master.tar.gz -C /libmpdclient-dist .
COPY . /myMPD/
ENV DESTDIR=/myMPD-dist
RUN mkdir -p $DESTDIR
WORKDIR /myMPD
RUN ./build.sh releaseinstall
WORKDIR /
RUN tar -czvf /mympd.tar.gz -C /myMPD-dist .

FROM library/debian:9-slim
ENV MPD_HOST=127.0.0.1
ENV MPD_PORT=6600
RUN apt-get update \
  && apt-get install -y --no-install-recommends openssl libmediainfo0v5 \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*
COPY --from=build /libmpdclient-master.tar.gz /
COPY --from=build /mympd.tar.gz /
WORKDIR /
RUN tar -xzvf libmpdclient-master.tar.gz -C /
RUN tar -xzvf mympd.tar.gz -C /
RUN rm libmpdclient-master.tar.gz
RUN rm mympd.tar.gz
RUN groupadd -r mympd
RUN useradd -r -g mympd -s /bin/false -d /var/lib/mympd mympd
ENTRYPOINT ["/usr/bin/mympd"]
