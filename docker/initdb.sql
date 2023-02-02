\c drgb_db drgb_user
--Check for tables
DO $$
  <<create_table>>
DECLARE
  mytable RECORD;
BEGIN
    SELECT * INTO mytable FROM pg_tables WHERE tablename='userlist';
    IF NOT FOUND THEN
    CREATE TABLE userlist(
      id BIGSERIAL PRIMARY KEY,
      register_time TIMESTAMP NOT NULL DEFAULT now(),
      access_time TIMESTAMP NOT NULL DEFAULT now(),
      email VARCHAR(50) NOT NULL UNIQUE,
      pwd_hash CHAR(64) NOT NULL,    --sha256 == 256bit == 32bytes == 64 letters in hex. IF you use just byte stream, use BYTEA Type.
  	win_count BIGINT DEFAULT 0,
  	lose_count BIGINT DEFAULT 0,
	draw_count BIGINT DEFAULT 0
  );
END IF;
END create_table $$;