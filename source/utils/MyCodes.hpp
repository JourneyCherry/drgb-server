#pragma once
#include "MyTypes.hpp"

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

//Request from Client
static constexpr byte REQ_REGISTER = 20;
static constexpr byte REQ_LOGIN = 21;
static constexpr byte REQ_CHPWD = 22;
static constexpr byte REQ_STARTMATCH = 23;
static constexpr byte REQ_PAUSEMATCH = 24;
static constexpr byte REQ_GAME_ACTION = 25;

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


//Inquiry among Servers
static constexpr byte INQ_COOKIE_CHECK = 40;	//쿠키 존재/만료 여부 확인.
static constexpr byte INQ_ACCOUNT_CHECK = 41;	//계정 접속/만료 여부 확인.
static constexpr byte INQ_COOKIE_TRANSFER = 42;	//쿠키 전달. 
static constexpr byte INQ_AVAILABLE = 43;		//가용량 확인.
static constexpr byte INQ_MATCH_TRANSFER = 44;	//매치 전달. Account_ID_t, Hash_t(cookie), Account_ID_t, Hash_t(cookie) 순으로 따라온다.