#pragma once
#include "MyTypes.hpp"

//Achievement List	//Achievement List는 파일을 통한 동적 변경이 불가능하다. 내부 로직에서 Ticking 로직을 변경할 수 없기 때문.
static constexpr Achievement_ID_t ACHIEVE_NEWBIE = 1;
static constexpr Achievement_ID_t ACHIEVE_MEDITATE_ADDICTION = 2;
static constexpr Achievement_ID_t ACHIEVE_GUARD_ADDICTION = 3;
static constexpr Achievement_ID_t ACHIEVE_EVADE_ADDICTION = 4;
static constexpr Achievement_ID_t ACHIEVE_PUNCH_ADDICTION = 5;
static constexpr Achievement_ID_t ACHIEVE_FIRE_ADDICTION = 6;
static constexpr Achievement_ID_t ACHIEVE_NOIVE = 7;
static constexpr Achievement_ID_t ACHIEVE_CHALLENGER = 8;
static constexpr Achievement_ID_t ACHIEVE_DOMINATOR = 9;
static constexpr Achievement_ID_t ACHIEVE_SLAYER = 10;
static constexpr Achievement_ID_t ACHIEVE_CONQUERER = 11;
static constexpr Achievement_ID_t ACHIEVE_KNEELINMYSIGHT = 12;
static constexpr Achievement_ID_t ACHIEVE_DOPPELGANGER = 13;
static constexpr Achievement_ID_t ACHIEVE_AREYAWINNINGSON = 14;

//Error Codes
static constexpr errorcode_t SUCCESS = 0;
static constexpr errorcode_t ERR_PROTOCOL_VIOLATION = 10;		//프로토콜 위반
static constexpr errorcode_t ERR_NO_MATCH_ACCOUNT = 11;		//일치하는 계정 없음
static constexpr errorcode_t ERR_EXIST_ACCOUNT = 12;			//이미 일치하는 계정 존재함.
static constexpr errorcode_t ERR_EXIST_ACCOUNT_MATCH = 13;		//이미 일치하는 계정 존재함 + 그 계정은 match서버에 있음.
static constexpr errorcode_t ERR_EXIST_ACCOUNT_BATTLE = 14;	//이미 일치하는 계정 존재함 + 그 계정은 battle서버에 있음.
static constexpr errorcode_t ERR_OUT_OF_CAPACITY = 15;			//대상 서버의 수용 가능 클라이언트 수 초과.
static constexpr errorcode_t ERR_DB_FAILED = 16;				//DB 관련 에러. 클라이언트 측에선 연결끊기 외엔 할 수 있는게 없다.
static constexpr errorcode_t ERR_DUPLICATED_ACCESS = 17;		//중복 접근. 보통 기존에 있는 세션에 보내지는 에러.
static constexpr errorcode_t ERR_CONNECTION_CLOSED = 18;
static constexpr errorcode_t ERR_TIMEOUT = 19;
static constexpr errorcode_t ERR_NO_MATCH_KEYWORD = ERR_NO_MATCH_ACCOUNT;

//Request from Client
static constexpr byte REQ_REGISTER = 20;
static constexpr byte REQ_LOGIN = 21;
static constexpr byte REQ_CHPWD = 22;
static constexpr byte REQ_STARTMATCH = 23;
static constexpr byte REQ_PAUSEMATCH = 24;
static constexpr byte REQ_GAME_ACTION = 25;
static constexpr byte REQ_CHNAME = 26;

//Answer to Client
static constexpr byte ANS_MATCHMADE = 30;
//Notify about game to Client
static constexpr byte GAME_ROUND_RESULT = 31;
static constexpr byte GAME_FINISHED_WIN = 32;
static constexpr byte GAME_FINISHED_DRAW = 33;
static constexpr byte GAME_FINISHED_LOOSE = 34;
static constexpr byte GAME_CRASHED = 35;	//게임이 터짐. 지금은 상대가 닷지하는 경우지만, 어뷰징을 방지하기 위해, 추후엔 초반 라운드에 닷지하는 경우 외엔 이 패킷을 없애야 한다.
static constexpr byte GAME_PLAYER_DISCONNEDTED = 36;
static constexpr byte GAME_PLAYER_ALL_CONNECTED = 37;
static constexpr byte GAME_PLAYER_INFO = 38;		//플레이어 정보. 닉네임 및 업적 내용 포함.
static constexpr byte GAME_PLAYER_INFO_NAME = 39;	//플레이어의 이름 정보.
static constexpr byte GAME_PLAYER_ACHIEVE = 40;		//도전과제 달성 알림. 요구 횟수를 충족했을 때만 온다.

//Inquiry among Servers
static constexpr byte INQ_REQUEST = 50;			//Inquiry애 대한 질문 헤더.
static constexpr byte INQ_ANSWER = 51;			//Inquiry에 대한 답 헤더.
static constexpr byte INQ_COOKIE_CHECK = 52;	//쿠키 존재/만료 여부 확인.
static constexpr byte INQ_ACCOUNT_CHECK = 53;	//계정 접속/만료 여부 확인.
static constexpr byte INQ_COOKIE_TRANSFER = 54;	//쿠키 전달. 
static constexpr byte INQ_MATCH_TRANSFER = 55;	//매치 전달. Account_ID_t, Hash_t(cookie), Account_ID_t, Hash_t(cookie) 순으로 따라온다.
static constexpr byte INQ_USAGE = 56;			//사용량(서버 내 자원) 확인.
static constexpr byte INQ_CLIENTUSAGE = 57;		//사용량(클라이언트 접속량) 확인. 동시접속자 수와 동일
static constexpr byte INQ_CONNUSAGE = 58;		//사용량(Connector 접속량) 확인.
static constexpr byte INQ_SESSIONS = 59;		//세션 수. (세션 수 - 동시접속자 수) == 대기 세션(쿠키만 할당 받고 아직 접속하지 않은 사용자 수)