FROM devkitpro/devkitppc:20240609

# Build latest wut
RUN \
mkdir wut && \
cd wut && \
git init . && \
git remote add origin https://github.com/devkitPro/wut.git && \
git fetch --depth 1 origin f17054e3e86222c14a1094639558d34b690636ed && \
git checkout FETCH_HEAD
WORKDIR /wut
RUN make -j$(nproc)
RUN make install

# Make git happy
RUN git config --global --add safe.directory /project

WORKDIR /project
