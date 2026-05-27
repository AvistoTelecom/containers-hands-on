# Exercice C

## Introduction

This exercise focuses on writing a **production-quality Containerfile**. You will build a small C++ application that uses an external library, and apply the following best practices along the way.

| Practice | Why |
|---|---|
| Pin image tags and digests | Reproducible builds — a tag can be overwritten; a digest is immutable |
| `--no-install-recommends` | Avoids pulling in suggested packages that bloat the image |
| Clean the apt cache in the same `RUN` | Cache written and deleted in the same layer costs zero bytes in the final image |
| `WORKDIR` | Avoids operations at `/`; makes paths explicit |
| Multi-stage build | Build tools never reach the final image |
| `COPY --from` | Pulls only the compiled artifact from the builder stage |
| Non-root user | Limits blast radius if the process is compromised |

## The application

`src/main.cpp` is a small C++ program that uses **libcurl** to print its own version at runtime. You need to compile it with:

```bash
g++ -o app main.cpp -lcurl
```

## Dependencies

Use these apt packages in your `Containerfile`. Figure out in which stage each one belongs.

**To build:**
- `g++`
- `libcurl4-openssl-dev`

**To run:**
- `libcurl4`

## Project structure

```
exercice-c/
├── Containerfile              ← you will create this
├── Containerfile-correction   ← reference solution
├── .dockerignore
└── src/
    └── main.cpp
```

## Instructions

### 1. Choose a base image

Pull two variants of the same Debian image and compare their sizes.

```bash
docker pull debian:12
docker pull debian:12-slim
docker images debian
```

`debian:12-slim` strips documentation, locale data, and other non-essential files. Prefer slim (or distroless/alpine) for production images. Always **pin the tag** — never use `latest`.

A tag like `debian:12-slim` can silently be overwritten by a new push. For fully reproducible builds, also pin the image to its **digest** (a SHA256 hash of the image manifest, immutable by design).

Fetch the digest of your local image with:

```bash
docker inspect --format='{{index .RepoDigests 0}}' debian:12-slim
```

The output looks like:

```
debian@sha256:3d868b5eb908155f3784317b3dda2941df87bbbbaa4608f84881de66d9bb297
```

Use it in your `FROM` instruction alongside the tag, so the file stays human-readable:

```dockerfile
FROM debian:12-slim@sha256:3d868b5eb908155f3784317b3dda2941df87bbbbaa4608f84881de66d9bb297 AS builder
```

Update both `FROM` lines in your `Containerfile` with the digest you just retrieved.

### 2. Write your Containerfile

Create a `Containerfile` in this directory. Your file must use **two named stages**: one for building, one for running.

Requirements:
- Base image: `debian:12-slim` for both stages
- Install build dependencies only in the builder stage, runtime dependency only in the runtime stage
- In each `RUN` that uses `apt-get`, install packages with `--no-install-recommends` and clean the cache (`rm -rf /var/lib/apt/lists/*`) **in the same instruction**
- Set a `WORKDIR` in each stage
- Use `COPY --from=<stage>` to bring the compiled binary into the runtime stage
- Create a non-root system user and switch to it before `ENTRYPOINT`
- Set the `ENTRYPOINT` to run the compiled binary

### 3. Build and compare stage sizes

Build the full image, then build only the builder stage to compare sizes.

```bash
docker build -f Containerfile -t my-app:final .
docker build -f Containerfile --target builder -t my-app:builder .
docker images my-app
```

The builder image carries the compiler and dev headers. The final image contains only the binary and the runtime library — it should be significantly smaller.

### 4. Verify the non-root user

Run the container normally, then override the entrypoint to check which user is running.

```bash
docker run --rm my-app:final
docker run --rm --entrypoint whoami my-app:final
```

The second command should print `appuser`, not `root`.

### 5. Understand why the cache must be cleaned in the same layer

Try splitting the apt install into two separate `RUN` instructions in your builder stage:

```dockerfile
RUN apt-get update && apt-get install -y --no-install-recommends g++ libcurl4-openssl-dev
RUN rm -rf /var/lib/apt/lists/*
```

Rebuild and compare the image size against the version where both commands are in the same `RUN`.

The cache files are committed to the first layer before the second `RUN` has a chance to delete them — they remain in the image even though they are not visible at runtime.

### 6. Export the image to a tar archive

`docker save` serialises an image — including all its layers, metadata, and tags — into a single `.tar` file. This is useful for transferring images to machines with no registry access, archiving a specific version, or inspecting the raw layer contents.

```bash
docker save my-app:final -o my-app-final.tar
```

Inspect what is inside the archive:

```bash
tar -tf my-app-final.tar
```

You will see one directory per layer (each containing a `layer.tar` with the actual filesystem diff), a `manifest.json` describing the image, and a JSON config file holding the image metadata (env, entrypoint, layer chain).

To load the image back on any Docker host:

```bash
docker load -i my-app-final.tar
```

Remove the archive once you are done:

```bash
rm my-app-final.tar
```

> `docker save` / `docker load` operate on images. To export the **filesystem of a running container** instead, use `docker export` / `docker import` — but that discards layer history and metadata.

---

## Cleanup

Remove the images built during the exercise.

```bash
docker rmi my-app:final my-app:builder
```

---

> **Key takeaway**: multi-stage builds keep build tools out of production images, and a non-root user limits what an attacker can do if the process is ever exploited. Both cost almost nothing to set up.
