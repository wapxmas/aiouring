FROM nexus.head-point.ru:5000/archlinux:base-devel as base
WORKDIR /app

RUN pacman -Syy
RUN pacman -Syu --noconfirm postgresql-libs mimalloc mold ccache cmake bash liburing

FROM base as build
WORKDIR /app

COPY etc ./etc
COPY include ./include
COPY libs ./libs
COPY CMakeLists.txt ./CMakeLists.txt
COPY VSBalancer.cpp ./VSBalancer.cpp
COPY VSBalancer.h.in ./VSBalancer.h.in
COPY compile.sh ./compile.sh

RUN ./compile.sh

FROM base as final

WORKDIR /app

COPY --from=build /app/buildbin/bin .

ENV LD_LIBRARY_PATH "/app:$LD_LIBRARY_PATH"

ENTRYPOINT ["./vs-balancer"]
