#!/bin/bash
if [ $1 = "-i" ]; then
    rm -rf /tmp/dcastle
fi
mkdir -p /tmp/dcastle
podman build --file Dockerfile.devel --tag darkcastle-devel --volume "/tmp/dcastle:/srv/dcastle"
podman build --file Dockerfile.prod --tag darkcastle-prod --volume "/tmp/dcastle:/srv/dcastle"
podman image save localhost/darkcastle-prod -o build/darkcastle-prod.img
xz --compress build/darkcastle-prod.img > build/darkcastle-prod.img.xz