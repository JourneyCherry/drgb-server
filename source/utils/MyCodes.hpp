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
static constexpr byte ANS_HEARTBEAT = 29;	//Hearbeat. Bi-directional
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
