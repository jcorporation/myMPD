FROM library/debian:9 as build
RUN apt-get update
RUN apt-get install git meson ninja-build gcc cmake libssl-dev -y
RUN apt-get install openjdk-8-jre-headless perl -y
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
ENV MYMPD_INSTALL_PREFIX=/myMPD-dist/usr
RUN mkdir -p $MYMPD_INSTALL_PREFIX
WORKDIR /myMPD
RUN ./mkrelease.sh
WORKDIR /
RUN tar -czvf /mympd.tar.gz -C /myMPD-dist .

FROM library/debian:9-slim
ENV MYMPD_COVERIMAGENAME=folder.jpg
ENV MYMPD_MPDHOST=127.0.0.1
ENV MYMPD_MPDPORT=6600
ENV MYMPD_SSL=true
ENV MYMPD_LOGLEVEL=1
RUN apt-get update && apt-get install libssl-dev openssl -y
COPY --from=build /libmpdclient-master.tar.gz /
COPY --from=build /mympd.tar.gz /
COPY --from=build /myMPD/debian/postinst /
WORKDIR /
RUN tar xfv libmpdclient-master.tar.gz -C /
RUN tar xfv mympd.tar.gz -C /
RUN rm libmpdclient-master.tar.gz
RUN rm mympd.tar.gz
COPY contrib/docker/init.sh /
RUN chmod +x /init.sh
ENTRYPOINT ["/init.sh"]
