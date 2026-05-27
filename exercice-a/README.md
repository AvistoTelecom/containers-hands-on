# Exercice A

## Introduction

This exercice presents basics of containers using Docker.

## Instructions

### 1. Build the image

Build a Docker image from the `Containerfile` in the current directory. The `-f` flag specifies the file and `-t` gives it a name.

```bash
docker build -f Containerfile -t my-image .
```

The `Containerfile` is minimal: it starts from `alpine` (a tiny Linux base image) and runs `sleep 3600` to keep the container alive, we'll see in the next sections how a `Containerfile` works.

### 2. List local images

Verify the image was built. This command shows all images available on your machine.

```bash
docker images
```

You should see `my-image` listed with its size and ID.

### 3. Run two containers from the same image

Start a first container in detached mode (`-d`) so it runs in the background. The `--name` flag gives it a recognizable name.

```bash
docker run -d --name container1 my-image
```

Start a second container the same way with a different name.

```bash
docker run -d --name container2 my-image
```

### 4. List running containers

Confirm both containers are up. This command shows all currently running containers.

```bash
docker ps
```

You should see `container1` and `container2` listed with their status. Both are running the same image — two isolated instances from a single template.

### 5. Process isolation between containers

Each container runs in its own **PID namespace**: it can only see its own processes, not those of other containers.

Run `ps` inside `container1`:

```bash
docker exec container1 ps aux
```

You should see only one process — the `sleep 3600` that belongs to that container. `container2` is invisible from here.

Do the same inside `container2`:

```bash
docker exec container2 ps aux
```

Same result: only its own `sleep` process.

Now check from the **host machine**:

```bash
ps aux | grep sleep
```

From the host, both `sleep` processes are visible — the kernel runs them all, but each container only has a window onto its own slice.

---

## Cleanup

Stop and remove the two containers, then remove the image.

```bash
docker rm -f container1 container2
docker rmi my-image
```

---

## Troubleshooting

If Docker does not seem to work at all, run the smoke-test image to verify your installation is functional.

```bash
docker run hello-world
```

A successful output means Docker can pull images and run containers correctly.
