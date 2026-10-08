-- 将歌曲专辑从冗余字符串迁移为多首歌曲关联一个专辑实体。
-- 保留 song.album / song.cover_hash 以兼容旧客户端与旧封面数据。
CREATE TABLE IF NOT EXISTS album (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(128) NOT NULL,
    cover_hash CHAR(64),
    cover_format VARCHAR(10),
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted TINYINT(1) NOT NULL DEFAULT 0,
    UNIQUE KEY uk_album_name (name)
) ENGINE=InnoDB;

ALTER TABLE song ADD COLUMN album_id BIGINT NULL AFTER artist;

INSERT INTO album (name)
SELECT TRIM(album)
FROM song
WHERE album IS NOT NULL AND TRIM(album) <> '' AND deleted = 0
GROUP BY TRIM(album)
ON DUPLICATE KEY UPDATE name = VALUES(name);

UPDATE song s
JOIN album a ON a.name = TRIM(s.album)
SET s.album_id = a.id
WHERE s.album IS NOT NULL AND TRIM(s.album) <> '' AND s.deleted = 0;

ALTER TABLE song
    ADD KEY ix_song_album_id (album_id),
    ADD CONSTRAINT fk_song_album FOREIGN KEY (album_id) REFERENCES album(id);
