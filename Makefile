all: client server

CLIENT_DIR := src/client
SERVER_DIR := src/server

client:
	$(MAKE) -C $(CLIENT_DIR)

server:
	$(MAKE) -C $(SERVER_DIR)

re: clean all

clean:
	$(MAKE) -C $(CLIENT_DIR) fclean
	$(MAKE) -C $(SERVER_DIR) fclean
