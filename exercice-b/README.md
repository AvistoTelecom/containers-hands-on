# Exercice B

## Introduction

This exercise introduces **Dockerfiles** and the concept of **layer caching**. Each instruction in a Dockerfile creates a layer. Docker caches these layers and only rebuilds from the first layer that changed — making subsequent builds much faster.

## The Containerfile

The `Containerfile` in this directory uses four instructions:

| Instruction | Role |
|---|---|
| `FROM` | Sets the base image |
| `RUN` | Executes a command during the build (here, a slow 10-second step simulating a heavy operation like installing dependencies) |
| `COPY` | Copies a file from your machine into the image |
| `ENTRYPOINT` | Defines the command that runs when a container starts |

## Project structure

```
exercice-b/
├── Containerfile
├── .dockerignore
├── secret.txt
└── src/
    └── hello.sh
```

## Instructions

### 1. Build the image

Build a Docker image from the `Containerfile` in the current directory. The `-f` flag specifies the file and `-t` tags it with a name and version.

```bash
docker build -f Containerfile -t my-app:v1 .
```

Notice that the `RUN sleep 10` step takes about 10 seconds.

### 2. Understand the build context

The `.` at the end of `docker build` is the **build context**: the directory whose contents are sent to the Docker daemon before the build starts. All paths in `COPY` instructions are resolved relative to this root.

Since `hello.sh` lives in `src/`, the Containerfile references it as `src/hello.sh`:

```dockerfile
COPY src/hello.sh /hello.sh
```

If you move, rename the file, or wish to change the context, the path in `COPY` must be updated accordingly.

### 3. Experiment with context size

The context is transferred in full before the build starts — even files that no `COPY` instruction ever uses. This is the behavior of the classic builder. BuildKit (the default since Docker 23) optimises this by only transferring files referenced in `COPY` instructions, so unreferenced files are silently skipped.

To observe the classic behavior, disable BuildKit explicitly:

**Too big** — create a large dummy file and rebuild with the classic builder:

```bash
dd if=/dev/zero of=bigfile.bin bs=1M count=50
DOCKER_BUILDKIT=0 docker build -f Containerfile -t my-app:v1 .
```

Watch the first line of the build output: `Sending build context to Docker daemon  52.43MB`. The daemon received 50 MB it will never use. Delete the file once you have seen the effect.

```bash
rm bigfile.bin
```

**Too narrow** — try passing `src/` as the context root instead of `.`:

```bash
docker build -f Containerfile -t my-app:test src/
```

Docker fails: `COPY src/hello.sh /hello.sh` looks for `src/hello.sh` relative to the context root, which is now `src/` — there is no `src/` subdirectory inside it. Narrowing the context root shifts all paths, breaking any `COPY` that expected the original layout.

### 4. Exclude files with .dockerignore

The `.dockerignore` file works exactly like `.gitignore`: files and directories matching its patterns are excluded from the build context. They are never sent to the daemon and cannot be referenced in `COPY` instructions.

```
# .dockerignore
secret.txt
*.md
```

To see this in action, temporarily add the following line to the `Containerfile` and try to rebuild:

```dockerfile
COPY secret.txt /secret.txt
```

Docker will fail with an error — `secret.txt` is not part of the context. Remove that line before continuing.

### 5. Rebuild without any change

Run the exact same build command again.

```bash
docker build -f Containerfile -t my-app:v1 .
```

The build is now **instant**. Docker reused all cached layers — nothing changed, so nothing was rebuilt.

### 6. Modify the script and rebuild

Edit `src/hello.sh` — change the message to anything you like, then build a new version.

```bash
docker build -f Containerfile -t my-app:v2 .
```

The build is fast again: Docker detects that only the `COPY` layer changed and reuses the cached `RUN` layer. The slow 10-second step is skipped.

### 7. Compare layer SHAs between the two versions

Each layer is identified by a SHA256 hash of its content. Inspect the layer stack of both images and compare them.

```bash
docker inspect --format='{{json .RootFS.Layers}}' my-app:v1 | tr ',' '\n'
docker inspect --format='{{json .RootFS.Layers}}' my-app:v2 | tr ',' '\n'
```

You should see that the first layers (`FROM` and `RUN`) share the **same SHA** — they were not rebuilt. Only the last layer (`COPY`) has a **different SHA**, because `src/hello.sh` changed.

### 8. Understand cache invalidation

Now move the `COPY` instruction **above** the `RUN` in the `Containerfile` so it looks like this:

```dockerfile
FROM alpine
COPY src/hello.sh /hello.sh
RUN sleep 10 && echo "Heavy setup done"
ENTRYPOINT ["sh", "/hello.sh"]
```

Rebuild, then edit `src/hello.sh` again and rebuild once more.

```bash
docker build -f Containerfile -t my-app:v2 .
```

This time the slow `RUN` step runs again. Because `COPY` now comes before `RUN`, any change to `src/hello.sh` **busts the cache for every layer below it**, including the expensive one.

### 9. Explore the layers

List every layer of your image with its size and the instruction that created it.

```bash
docker history my-app:v1
```

Add `--no-trunc` to see the full commands, useful to compare directly with your Containerfile.

```bash
docker history --no-trunc my-app:v1
```

---

## Cleanup

Remove both image versions built during the exercise.

```bash
docker rmi my-app:v1 my-app:v2
```

---

> **Key takeaway**: put instructions that change rarely (installs, heavy setup) **before** instructions that change often (copying your source code). Use `.dockerignore` to keep sensitive files and build noise out of the context.
