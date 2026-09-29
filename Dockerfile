FROM devkitpro/devkitppc:20260221
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260418 /artifacts $DEVKITPRO
WORKDIR /project
