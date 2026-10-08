-- 支持同名专辑由不同艺术家发行，并以专辑名 + 专辑艺术家作为唯一身份。
ALTER TABLE album
    ADD COLUMN artist VARCHAR(128) NOT NULL DEFAULT '' AFTER name,
    DROP INDEX uk_album_name,
    ADD UNIQUE KEY uk_album_name_artist (name, artist);
