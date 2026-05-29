# Etapa 1: build

FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    flex \
    bison \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copiar fuentes
COPY . .

# Compilar
RUN make

# Etapa 2: imagen final (más liviana, sin herramientas de build)

FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copiar solo el ejecutable compilado
COPY --from=builder /build/simple .

# Directorio donde el usuario puede montar sus archivos .ci
VOLUME ["/app/programs"]

# El compilador lee de stdin, así que el entrypoint
# permite pasarle el archivo directamente
ENTRYPOINT ["./simple"]
