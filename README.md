# Lightweight C++ Web Server for Docker Stats
A minimal C++ web server that queries the **Docker daemon** to:

- **List running containers** (`GET /containers`)
- **Fetch stats for specific containers** (`GET /container_stats?ids=container_id1,container_id2`)

Runs in an **Alpine Linux Docker container**, supports **Unix sockets & exposed daemons**

### Dependencies & Licensing

This project uses two external headers:

- **[cpp-httplib](https://github.com/yhirose/cpp-httplib) by yhirose**
   - Source: [GitHub](https://github.com/yhirose/cpp-httplib)

- **[json.hpp (nlohmann/json)](https://github.com/nlohmann/json) by Niels Lohmann**
   - Source: [GitHub](https://github.com/nlohmann/json)


### Instructions without Docker Compose

```shell
docker build -t docker-stats-web-server .
```

#### On a Linux Host (Not Docker Desktop)

```shell
docker run -it --rm \
   -p 8080:8080 \
   -v /var/run/docker.sock:/var/run/docker.sock \
   docker-stats-web-server:latest
```

#### Docker Desktop (Windows, Mac, Linux)

(powershell example)
1. Expose Docker Daemon
2. ```shell
   docker run -it --rm -p 8080:8080 `
    -e DOCKER_ENDPOINT="host.docker.internal:2375" `
    docker-stats-web-server:latest
    ```

---

### Instructions with Docker Compose

The compose file will build (or pull) the `docker-stats-web-server:latest` image if it does not exist yet.
Depending on the machine (using Docker Desktop or not) comment in and out the volume or the environment.

```shell
docker compose up -d
```

---


### HTTP API

Once the container is running:

1. List containers:
    ```shell
    curl http://localhost:8080/containers
    ```

2. Container stats (provide comma-separated IDs):
    ```shell
    curl http://localhost:8080/container_stats?ids=<id1>,<id2>
    ```

It should return JSON results in the same structure as the official docker api.
