FROM gcc:latest
WORKDIR /app
COPY . .
RUN make server -f Makefile
CMD ["./src/server/bin/ftp_server"]