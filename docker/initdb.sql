\c drgb_db drgb_user
--Check for tables
DO $$
  <<create_table>>
DECLARE
  mytable RECORD;
BEGIN
  --Achievement Table--
  SELECT * INTO mytable FROM pg_tables WHERE tablename='achievement';
  IF NOT FOUND THEN
    CREATE TABLE achievement(
      id BIGSERIAL PRIMARY KEY,
      count INT NOT NULL DEFAULT 0
    );
	--ALTER SEQUENCE public.achievement_id_seq RESTART WITH 0;
		--Cannot alter sequence under MINVALUE(1)
    INSERT INTO achievement (count) VALUES
      (0),  --Newbie
      (1),  --Meditate Addiction
      (3),  --Guard Addiction
      (2),  --Evade Addiction
      (2),  --Punch Addiction
      (5),  --Fire Addiction
      (1),  --Novice
      (5),  --Challenger
      (10), --Dominator
      (15), --Slayer
      (30), --,Conqueror
      (1),  --,Kneel in my sight
      (1),  --,Doppelganger
      (1);  --,Are Ya Winning Son?
  END IF;

  --User List Table--
  SELECT * INTO mytable FROM pg_tables WHERE tablename='userlist';
  IF NOT FOUND THEN
    CREATE TABLE userlist(
      id BIGSERIAL PRIMARY KEY,
      register_time TIMESTAMP NOT NULL DEFAULT now(),
      access_time TIMESTAMP NOT NULL DEFAULT now(),
      nickname BIGINT DEFAULT 1 REFERENCES achievement(id) ON DELETE SET NULL,
      email VARCHAR(50) NOT NULL UNIQUE,
      pwd_hash CHAR(64) NOT NULL,    --sha256 == 256bit == 32bytes == 64 letters in hex. IF you use just byte stream, use BYTEA Type.
      win_count BIGINT DEFAULT 0,
      draw_count BIGINT DEFAULT 0,
      loose_count BIGINT DEFAULT 0
    );
  END IF;

  --User Achievement Table--
  SELECT * INTO mytable FROM pg_tables WHERE tablename='user_achievement';
  IF NOT FOUND THEN
    CREATE TABLE user_achievement(
      user_id BIGINT NOT NULL REFERENCES userlist(id) ON DELETE CASCADE,
      achievement_id BIGINT NOT NULL REFERENCES achievement(id) ON DELETE CASCADE,
      count INT NOT NULL DEFAULT 0,
      PRIMARY KEY (user_id, achievement_id)
    );
  END IF;
END create_table $$;