CREATE DATABASE IF NOT EXISTS futari CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE futari;

CREATE TABLE IF NOT EXISTS user (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(32) NOT NULL,
    nickname VARCHAR(32) NOT NULL,
    avatar_url VARCHAR(255),
    password_hash VARCHAR(100) NOT NULL,
    role VARCHAR(10) NOT NULL DEFAULT 'USER',
    can_upload TINYINT(1) NOT NULL DEFAULT 0,
    can_manage_server_playlist TINYINT(1) NOT NULL DEFAULT 0,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted TINYINT(1) NOT NULL DEFAULT 0,
    UNIQUE KEY uk_user_username (username),
    KEY ix_user_nickname (nickname)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS album (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(128) NOT NULL,
    artist VARCHAR(128) NOT NULL DEFAULT '',
    cover_hash CHAR(64),
    cover_format VARCHAR(10),
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted TINYINT(1) NOT NULL DEFAULT 0,
    UNIQUE KEY uk_album_name_artist (name, artist)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS song (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    hash CHAR(64) NOT NULL,
    title VARCHAR(128) NOT NULL,
    artist VARCHAR(128),
    album_id BIGINT,
    album VARCHAR(128),
    lyrics MEDIUMTEXT,
    has_lyrics TINYINT(1) NOT NULL DEFAULT 0,
    cover_hash CHAR(64),
    cover_format VARCHAR(10),
    duration_ms INT,
    file_size BIGINT NOT NULL,
    format VARCHAR(10) NOT NULL,
    uploader_id BIGINT NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted TINYINT(1) NOT NULL DEFAULT 0,
    UNIQUE KEY uk_song_hash (hash),
    KEY ix_song_title (title),
    KEY ix_song_album_id (album_id),
    CONSTRAINT fk_song_uploader FOREIGN KEY (uploader_id) REFERENCES user(id),
    CONSTRAINT fk_song_album FOREIGN KEY (album_id) REFERENCES album(id)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS server_playlist (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(64) NOT NULL,
    description VARCHAR(255),
    cover_url VARCHAR(255),
    creator_id BIGINT NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted TINYINT(1) NOT NULL DEFAULT 0,
    CONSTRAINT fk_server_playlist_creator FOREIGN KEY (creator_id) REFERENCES user(id)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS user_playlist (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    user_id BIGINT NOT NULL,
    name VARCHAR(64) NOT NULL,
    description VARCHAR(255),
    cover_url VARCHAR(255),
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted TINYINT(1) NOT NULL DEFAULT 0,
    KEY ix_user_playlist_user (user_id),
    CONSTRAINT fk_user_playlist_owner FOREIGN KEY (user_id) REFERENCES user(id)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS server_playlist_song (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    playlist_id BIGINT NOT NULL,
    song_id BIGINT NOT NULL,
    sort_order INT NOT NULL DEFAULT 0,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted TINYINT(1) NOT NULL DEFAULT 0,
    UNIQUE KEY uk_server_playlist_song (playlist_id, song_id),
    KEY ix_server_playlist_sort (playlist_id, sort_order),
    CONSTRAINT fk_server_entry_playlist FOREIGN KEY (playlist_id) REFERENCES server_playlist(id),
    CONSTRAINT fk_server_entry_song FOREIGN KEY (song_id) REFERENCES song(id)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS user_playlist_song (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    playlist_id BIGINT NOT NULL,
    song_id BIGINT NOT NULL,
    sort_order INT NOT NULL DEFAULT 0,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted TINYINT(1) NOT NULL DEFAULT 0,
    UNIQUE KEY uk_user_playlist_song (playlist_id, song_id),
    KEY ix_user_playlist_sort (playlist_id, sort_order),
    CONSTRAINT fk_user_entry_playlist FOREIGN KEY (playlist_id) REFERENCES user_playlist(id),
    CONSTRAINT fk_user_entry_song FOREIGN KEY (song_id) REFERENCES song(id)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS favorite_partner (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    user_id BIGINT NOT NULL,
    partner_id BIGINT NOT NULL,
    remark VARCHAR(32),
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted TINYINT(1) NOT NULL DEFAULT 0,
    UNIQUE KEY uk_favorite_partner (user_id, partner_id),
    KEY ix_favorite_target (partner_id),
    CONSTRAINT fk_favorite_owner FOREIGN KEY (user_id) REFERENCES user(id),
    CONSTRAINT fk_favorite_target FOREIGN KEY (partner_id) REFERENCES user(id)
) ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS room (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(64) NOT NULL,
    owner_id BIGINT NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    deleted TINYINT(1) NOT NULL DEFAULT 0,
    KEY ix_room_owner (owner_id),
    CONSTRAINT fk_room_owner FOREIGN KEY (owner_id) REFERENCES user(id)
) ENGINE=InnoDB;
