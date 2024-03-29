#include <memory>
#include "MyMain.hpp"
#include "MyAuth.hpp"

int main(int argc, char *argv[])
{
	std::shared_ptr<MyServer> server;
	MyMain starter("drgb-auth",
		[&]()
		{
			server = std::make_shared<MyAuth>();
			server->Start();
			server->Join();
		}, 
		[&]()
		{
			server->Stop();
		}
	);
	return starter.main(argc, argv);
}