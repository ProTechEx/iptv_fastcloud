FROM debian:stretch-slim

LABEL maintainer="Alexandr Topilski <support@fastogt.com>"

ENV USER fastocloud
ENV APP_NAME fastocloud
RUN groupadd -r $USER && useradd -r -g $USER $USER

RUN set -ex; \
  BUILD_DEPS='ca-certificates git python3 python3-pip'; \
  PREFIX=/usr/local; \
  apt-get update; \
  apt-get install -y $BUILD_DEPS --no-install-recommends; \
  # rm -rf /var/lib/apt/lists/*; \
  \
  pip3 install setuptools; \
  PYFASTOGT_DIR=/usr/src/pyfastogt; \
  mkdir -p $PYFASTOGT_DIR && git clone https://github.com/fastogt/pyfastogt $PYFASTOGT_DIR && cd $PYFASTOGT_DIR && python3 setup.py install; \
  \
  PROJECT_DIR=/usr/src/fastocloud; \
  mkdir -p $PROJECT_DIR && git clone https://github.com/fastogt/fastocloud $PROJECT_DIR && cd $PROJECT_DIR && git submodule update --init --recursive; \
  cd $PROJECT_DIR/build && ./build_env.py --prefix=$PREFIX; \
  LICENSE_KEY="$(license_gen --machine-id)"; \
  cd $PROJECT_DIR/build && PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig python3 build.py release $LICENSE_KEY 1 $PREFIX; \
rm -rf $PYFASTOGT_DIR $PROJECT_DIR # && apt-get purge -y --auto-remove $BUILD_DEPS

RUN mkdir /var/run/$APP_NAME && chown $USER:$USER /var/run/$APP_NAME
VOLUME /var/run/$APP_NAME
WORKDIR /var/run/$APP_NAME

ENTRYPOINT ["fastocloud"]

EXPOSE 6317 6000 7000 8000
