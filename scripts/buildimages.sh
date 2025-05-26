#!/bin/bash
export DCTMPDIR=$(mktemp -d)
echo $DCTMPDIR
podman build --file Dockerfile --tag darkcastle-devel --volume "${DCTMPDIR}:/hostdir"
podman build -f Dockerfile.prod --tag darkcastle-prod --volume ${DCTMPDIR}:/hostdir
podman image save localhost/darkcastle-prod -o build/darkcastle-prod.image
xz --compress build/darkcastle-prod.image > build/darkcastle-prod.image.xz