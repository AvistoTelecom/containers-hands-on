# Containers hands-on

A set of practical exercises to get familiar with Docker. Each exercise builds on the previous one and focuses on a specific set of concepts.

## Exercises

| Exercise | Topics |
|---|---|
| [A](exercice-a/README.md) | Build an image, list images, run multiple containers, process isolation |
| [B](exercice-b/README.md) | Containerfile instructions, build context, `.dockerignore`, layer caching, layer inspection |
| [C](exercice-c/README.md) | Multi-stage builds, base image selection, non-root user, image digest pinning |
| [D](exercice-d/README.md) | Named volumes, data persistence, user-defined networks, DNS resolution |
| [E](exercice-e/README.md) | Docker Compose, multi-service stacks, service dependencies, environment variables |

## Prerequisites

- Docker installed and running — verify with `docker run hello-world`
- Basic comfort with a terminal

## Useful links

- [Official documentation](https://docs.docker.com/)
- [Official cheatsheet](https://docs.docker.com/get-started/docker_cheatsheet.pdf)
- [Dockerfile reference](https://docs.docker.com/reference/dockerfile/)
- [Understanding kernel namespaces](https://gcore.com/learning/what-is-a-container-a-kernel-introduction/)
- [dive — explore image layers interactively](https://github.com/wagoodman/dive)
