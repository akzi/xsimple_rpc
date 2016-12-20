CFLAGS = -g -W \
-O3 \
-Wall \
-Wpointer-arith \
-Wshadow \
-Wno-long-long \
-Wformat \
-Winvalid-pch \
-std=c++1z \
-I ./../../include \
-l pthread
run:server
	./rpc_client
server:rpc_client
	g++ rpc_client.cpp $(CFLAGS) -o rpc_client
rm_server:
	$(shell if [ -f rpc_client]; then rm rpc_client; fi;)

