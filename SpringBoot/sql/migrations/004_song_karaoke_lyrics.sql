-- Keep line-synced lyrics in `lyrics`; store per-character LRC independently.
-- Place the nullable column at the end so MySQL 8 can use instant ADD COLUMN.
ALTER TABLE song
    ADD COLUMN karaoke_lyrics MEDIUMTEXT NULL,
    ALGORITHM=INSTANT;
