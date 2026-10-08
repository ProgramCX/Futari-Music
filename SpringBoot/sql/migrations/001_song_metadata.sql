-- 仅对已使用旧版 schema.sql 创建的数据库执行一次。
ALTER TABLE song
    ADD COLUMN album VARCHAR(128) NULL AFTER artist,
    ADD COLUMN lyrics MEDIUMTEXT NULL AFTER album,
    ADD COLUMN has_lyrics TINYINT(1) NOT NULL DEFAULT 0 AFTER lyrics,
    ADD COLUMN cover_hash CHAR(64) NULL AFTER has_lyrics,
    ADD COLUMN cover_format VARCHAR(10) NULL AFTER cover_hash;
