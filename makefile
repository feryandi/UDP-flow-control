default:
	g++ -pthread -o receiver dcomm.cpp receiver.cpp
	g++ -pthread -o sender sender.cpp
