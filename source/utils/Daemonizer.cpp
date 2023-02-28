#include "Daemonizer.hpp"

namespace mylib{
namespace utils{

bool Daemonizer()
{
	pid_t pid;
	pid = fork();	//0은 자식 프로세스, 0이상은 자식프로세스의 pid(즉, 부모프로세스), -1은 에러(자식 프로세스가 생성되지 않음)
	if(pid < 0)
		throw StackTraceExcept("Fork Failed : " + std::to_string(errno), __STACKINFO__);
	if(pid > 0)
		return false;

	if(setsid() == -1)
		throw StackTraceExcept("setsid() Failed", __STACKINFO__);

	signal(SIGHUP, SIG_IGN);

	pid = fork();
	if(pid < 0)
		throw StackTraceExcept("Fork Failed : " + std::to_string(errno), __STACKINFO__);
	if(pid > 0)
		return false;
	
	close(0);
	close(1);
	close(2);

	//chdir("/");	//Root Directory 변경. 필요한지 검토 필요.
	return true;
}

void Create_PidFile(std::string path)
{
	pid_t NowPid = getpid();

	std::ofstream pid_file(path, std::ios::out | std::ios::trunc);
	if(!pid_file.is_open())
		throw StackTraceExcept("Pid File Open Failed : " + std::to_string(errno), __STACKINFO__);
	pid_file << NowPid << std::endl;
	pid_file.close();
}

}
}