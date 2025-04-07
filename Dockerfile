FROM devkitpro/devkitppc:20250102

# Make git happy
RUN git config --global --add safe.directory /project

WORKDIR /project
