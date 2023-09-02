#include <memory>
#include "MyMain.hpp"
#include "MyMatch.hpp"

int main(int argc, char *argv[])
{
	std::shared_ptr<MyServer> server;
	MyMain starter("drgb-match",
		[&]()
		{
			server = std::make_shared<MyMatch>();
			server->Start();
			server->Join();
		}, 
		[&]()
		{
			server->Stop();
		});
	return starter.main(argc, argv);
}