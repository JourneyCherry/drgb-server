#include <memory>
#include "MyMain.hpp"
#include "MyBattle.hpp"

int main(int argc, char *argv[])
{
	std::shared_ptr<MyServer> server;
	MyMain starter(
		[&]()
		{
			server = std::make_shared<MyBattle>();
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