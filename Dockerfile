FROM alpine:3.18

# Install build tools
RUN apk add --no-cache g++ make musl-dev

WORKDIR /app

# Copy source and libraries
COPY main.cpp .
COPY httplib.h .
COPY json.hpp .

# Build the server
RUN g++ -std=c++11 main.cpp -o server -pthread

# Expose port 8080
EXPOSE 8080

# Default command
CMD ["./server"]
