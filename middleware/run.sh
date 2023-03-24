#!/bin/bash
docker stop mwc && docker rm mwc
docker run -d \
    --name mwc \
    --privileged \
    mwimg
docker logs -f mwc