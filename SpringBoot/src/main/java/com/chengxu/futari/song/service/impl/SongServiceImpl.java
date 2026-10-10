package com.chengxu.futari.song.service.impl;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.chengxu.futari.song.service.impl.CoverFiles.CoverPayload;
import static com.chengxu.futari.song.service.impl.CoverFiles.readCover;
import static com.chengxu.futari.song.service.impl.CoverFiles.storeCover;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.song.dto.SongResponse;
import com.chengxu.futari.song.dto.SongLyricsResponse;
import com.chengxu.futari.song.dto.SongCoverResponse;
import com.chengxu.futari.song.dto.SongFileResponse;
import com.chengxu.futari.song.dto.SongUploadRequest;
import com.chengxu.futari.song.dto.SongUpdateRequest;
import com.chengxu.futari.song.entity.Album;
import com.chengxu.futari.song.entity.Song;
import com.chengxu.futari.song.mapper.SongMapper;
import com.chengxu.futari.song.service.SongService;
import com.chengxu.futari.song.service.AlbumService;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.time.ZoneId;
import java.util.HexFormat;
import java.util.Set;
import java.util.Map;
import java.util.List;
import java.util.LinkedHashSet;
import java.util.Objects;
import java.util.stream.Collectors;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.dao.DuplicateKeyException;
import org.springframework.stereotype.Service;
import org.springframework.util.unit.DataSize;
import org.springframework.web.multipart.MultipartFile;

/** 本地内容寻址文件和歌曲元数据。 @author Futari */
@Slf4j @Service @RequiredArgsConstructor
public class SongServiceImpl implements SongService {
    private static final Set<String> FORMATS = Set.of("mp3", "flac", "aac", "ogg", "wav", "m4a");
    private static final Set<String> LYRIC_FORMATS = Set.of("lrc", "txt");
    private static final long MAX_LYRICS_BYTES = 256L * 1024;
    private static final int MAX_DURATION_MS = 24 * 60 * 60 * 1000;
    private final SongMapper mapper;
    private final AlbumService albumService;
    @Value("${futari.storage.directory}") private String storageDirectory;
    @Value("${futari.upload.max-audio-size}") private DataSize maxAudioSize;
    public Song require(Long id) {
        Song song = mapper.selectById(id);
        if (song == null) throw new BizException(ErrorCode.NOT_FOUND);
        return song;
    }
    public Song requireHash(String hash) {
        if (hash == null || !hash.matches("[a-f0-9]{64}")) throw new BizException(ErrorCode.NOT_FOUND);
        Song song = mapper.selectOne(new LambdaQueryWrapper<Song>().eq(Song::getHash, hash));
        if (song == null) throw new BizException(ErrorCode.NOT_FOUND);
        return song;
    }
    public SongResponse response(Song song) {
        Map<Long, Album> albums = song.getAlbumId() == null ? Map.of() : albumService.findByIds(Set.of(song.getAlbumId())).stream().collect(Collectors.toMap(Album::getId, album -> album));
        return response(song, albums);
    }
    @Override public List<SongResponse> findAvailableByIds(List<Long> songIds) {
        if (songIds.isEmpty()) return List.of();
        List<Song> available = mapper.selectBatchIds(new LinkedHashSet<>(songIds));
        Set<Long> albumIds = available.stream().map(Song::getAlbumId).filter(Objects::nonNull).collect(Collectors.toSet());
        Map<Long, Album> albums = albumService.findByIds(albumIds).stream().collect(Collectors.toMap(Album::getId, album -> album));
        Map<Long, Song> byId = available.stream().collect(Collectors.toMap(Song::getId, song -> song));
        // 软删除歌曲的关联保留，但不再使整个歌单失败；按歌单原顺序返回有效歌曲。
        return songIds.stream().map(byId::get).filter(Objects::nonNull).map(song -> response(song, albums)).toList();
    }
    private SongResponse response(Song song, Map<Long, Album> albums) {
        Long created = song.getCreatedAt() == null ? null : song.getCreatedAt().atZone(ZoneId.systemDefault()).toInstant().toEpochMilli();
        Album album = song.getAlbumId() == null ? null : albums.get(song.getAlbumId());
        String albumName = album == null ? song.getAlbum() : album.getName();
        String albumArtist = album == null ? null : album.getArtist();
        String coverUrl = album != null && album.getCoverHash() != null ? "/api/albums/" + album.getId() + "/cover"
                : song.getCoverHash() == null ? null : "/api/songs/" + song.getId() + "/cover";
        return new SongResponse(song.getId(), song.getHash(), song.getTitle(), song.getArtist(), song.getAlbumId(), albumName, albumArtist, Boolean.TRUE.equals(song.getHasLyrics()), coverUrl, song.getDurationMs(), song.getFileSize(), song.getFormat(), song.getUploaderId(), created);
    }
    public PageResult<SongResponse> list(String keyword, int pageNum, int pageSize) {
        if (pageNum < 1 || pageSize < 1 || pageSize > 100) throw new BizException(ErrorCode.PARAM);
        String value = keyword == null ? "" : keyword.trim();
        LambdaQueryWrapper<Song> query = new LambdaQueryWrapper<Song>().and(!value.isBlank(), q -> q.like(Song::getTitle, value).or().like(Song::getArtist, value).or().like(Song::getAlbum, value)).orderByDesc(Song::getId);
        Page<Song> page = mapper.selectPage(new Page<>(pageNum, pageSize), query);
        Set<Long> albumIds = page.getRecords().stream().map(Song::getAlbumId).filter(java.util.Objects::nonNull).collect(Collectors.toSet());
        Map<Long, Album> albums = albumService.findByIds(albumIds).stream().collect(Collectors.toMap(Album::getId, album -> album));
        return new PageResult<>(page.getTotal(), page.getRecords().stream().map(song -> response(song, albums)).toList());
    }
    @Override public PageResult<SongResponse> listMine(Long userId, int pageNum, int pageSize) {
        if (userId == null || pageNum < 1 || pageSize < 1 || pageSize > 100) throw new BizException(ErrorCode.PARAM);
        Page<Song> page = mapper.selectPage(new Page<>(pageNum, pageSize),
                new LambdaQueryWrapper<Song>().eq(Song::getUploaderId, userId).orderByDesc(Song::getId));
        Set<Long> albumIds = page.getRecords().stream().map(Song::getAlbumId).filter(java.util.Objects::nonNull).collect(Collectors.toSet());
        Map<Long, Album> albums = albumService.findByIds(albumIds).stream().collect(Collectors.toMap(Album::getId, album -> album));
        return new PageResult<>(page.getTotal(), page.getRecords().stream().map(song -> response(song, albums)).toList());
    }
    @org.springframework.transaction.annotation.Transactional(rollbackFor = Exception.class,
            isolation = org.springframework.transaction.annotation.Isolation.READ_COMMITTED)
    public SongResponse upload(Long userId, SongUploadRequest request) {
        MultipartFile file = request.getFile();
        if (file == null || file.isEmpty()) throw new BizException(ErrorCode.SONG_EMPTY);
        if (file.getSize() > maxAudioSize.toBytes()) throw new BizException(ErrorCode.SONG_UPLOAD_SIZE);
        String extension = extension(file);
        if (!FORMATS.contains(extension)) throw new BizException(ErrorCode.SONG_FORMAT);
        if (request.getTitle() == null || request.getTitle().isBlank() || request.getTitle().length() > 128 || request.getArtist() != null && request.getArtist().length() > 128 || request.getAlbum() != null && request.getAlbum().length() > 128 || invalidDuration(request.getDurationMs())) throw new BizException(ErrorCode.PARAM);
        String lyrics = readLyrics(request);
        MultipartFile albumCoverFile = request.getAlbumCoverFile() == null ? request.getCoverFile() : request.getAlbumCoverFile();
        CoverPayload albumCover = readCover(albumCoverFile);
        CoverPayload songCover = readCover(request.getSongCoverFile());
        if (request.getAlbumId() != null && request.getAlbumId() > 0 && request.getNewAlbumName() != null && !request.getNewAlbumName().isBlank()) throw new BizException(ErrorCode.PARAM);
        Path temporary = null;
        try {
            Path directory = Path.of(storageDirectory).toAbsolutePath().normalize();
            Files.createDirectories(directory);
            temporary = Files.createTempFile(directory, "upload-", ".tmp");
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            try (InputStream input = file.getInputStream(); var output = Files.newOutputStream(temporary)) {
                byte[] buffer = new byte[8192]; int count;
                while ((count = input.read(buffer)) != -1) { digest.update(buffer, 0, count); output.write(buffer, 0, count); }
            }
            String hash = HexFormat.of().formatHex(digest.digest());
            Song existing = mapper.selectOne(new LambdaQueryWrapper<Song>().eq(Song::getHash, hash));
            // 同一音频只对应一条全站元数据；重复上传不覆盖原歌手、歌词或封面。
            if (existing != null) return response(existing);
            Path target = directory.resolve(hash);
            try { Files.move(temporary, target, StandardCopyOption.ATOMIC_MOVE); temporary = null; }
            catch (FileAlreadyExistsException ex) { Files.deleteIfExists(temporary); temporary = null; }
            if (albumCover != null) storeCover(directory, albumCover);
            if (songCover != null) storeCover(directory, songCover);
            String albumName = blankToNull(request.getNewAlbumName());
            if (albumName == null && request.getAlbumId() == null) albumName = blankToNull(request.getAlbum());
            String albumArtist = blankToNull(request.getNewAlbumArtist());
            Album album = albumService.resolveForUpload(request.getAlbumId(), albumName, albumArtist,
                    albumCover == null ? null : albumCover.hash(), albumCover == null ? null : albumCover.format());
            Song song = new Song(); song.setHash(hash); song.setTitle(request.getTitle().trim()); song.setArtist(blankToNull(request.getArtist())); song.setAlbumId(album == null ? null : album.getId()); song.setAlbum(album == null ? null : album.getName()); song.setLyrics(lyrics); song.setHasLyrics(lyrics != null); song.setCoverHash(songCover == null ? null : songCover.hash()); song.setCoverFormat(songCover == null ? null : songCover.format()); song.setDurationMs(request.getDurationMs()); song.setFormat(extension); song.setFileSize(file.getSize()); song.setUploaderId(userId);
            song.setDeleted(0);
            Song deletedSong = mapper.selectDeletedByHash(hash);
            if (deletedSong != null) {
                song.setId(deletedSong.getId());
                if (mapper.restoreDeleted(song) == 0) return response(requireHash(hash));
                log.info("user {} restored uploaded song {}", userId, song.getId());
                return response(song);
            }
            try { mapper.insert(song); }
            catch (DuplicateKeyException ex) { return response(requireHash(hash)); }
            log.info("user {} uploaded song {}", userId, song.getId());
            return response(song);
        } catch (java.io.IOException | NoSuchAlgorithmException ex) {
            log.error("歌曲存储失败，用户 {}", userId, ex);
            throw new BizException(ErrorCode.SONG_STORAGE);
        } finally {
            if (temporary != null) {
                try { Files.deleteIfExists(temporary); } catch (java.io.IOException ex) { log.warn("临时文件清理失败: {}", temporary, ex); }
            }
        }
    }
    @Override @org.springframework.transaction.annotation.Transactional(rollbackFor = Exception.class)
    public SongResponse update(Long id, SongUpdateRequest request) {
        Song song = require(id);
        if (request.getTitle() == null || request.getTitle().isBlank() || request.getTitle().length() > 128 || request.getArtist() != null && request.getArtist().length() > 128 || invalidDuration(request.getDurationMs())) throw new BizException(ErrorCode.PARAM);
        if (request.getLyricsFile() != null || request.getLyricsText() != null) {
            String lyrics = readLyrics(request.getLyricsFile(), request.getLyricsText());
            song.setLyrics(lyrics); song.setHasLyrics(lyrics != null);
        }
        if (request.getFile() != null && !request.getFile().isEmpty()) {
            MultipartFile file = request.getFile();
            if (file.getSize() > maxAudioSize.toBytes()) throw new BizException(ErrorCode.SONG_UPLOAD_SIZE);
            String format = extension(file);
            if (!FORMATS.contains(format)) throw new BizException(ErrorCode.SONG_FORMAT);
            try {
                Path directory = Path.of(storageDirectory).toAbsolutePath().normalize();
                Files.createDirectories(directory);
                AudioPayload audio = storeAudio(file, directory);
                try {
                    Song duplicate = mapper.selectOne(new LambdaQueryWrapper<Song>().eq(Song::getHash, audio.hash()));
                    if (duplicate != null && !duplicate.getId().equals(id)) throw new BizException(ErrorCode.SONG_DUPLICATE);
                    Path target = directory.resolve(audio.hash());
                    if (!Files.exists(target)) Files.move(audio.temporary(), target, StandardCopyOption.ATOMIC_MOVE);
                    song.setHash(audio.hash()); song.setFormat(format); song.setFileSize(file.getSize());
                } finally { Files.deleteIfExists(audio.temporary()); }
            } catch (BizException ex) { throw ex; }
            catch (java.io.IOException | NoSuchAlgorithmException ex) { log.error("管理员替换歌曲音频失败，song {}", id, ex); throw new BizException(ErrorCode.SONG_STORAGE); }
        }
        if (request.getAlbumId() != null || request.getNewAlbumName() != null || request.getAlbumName() != null || request.getAlbumArtist() != null || request.getNewAlbumArtist() != null || request.getAlbumCoverFile() != null || request.getSongCoverFile() != null) {
            CoverPayload cover = readCover(request.getAlbumCoverFile());
            CoverPayload songCover = readCover(request.getSongCoverFile());
            if (songCover != null) {
                try { storeCover(Path.of(storageDirectory).toAbsolutePath().normalize(), songCover); }
                catch (java.io.IOException ex) { throw new BizException(ErrorCode.SONG_STORAGE); }
                song.setCoverHash(songCover.hash()); song.setCoverFormat(songCover.format());
            }
            if (cover != null) {
                try { storeCover(Path.of(storageDirectory).toAbsolutePath().normalize(), cover); }
                catch (java.io.IOException ex) { throw new BizException(ErrorCode.SONG_STORAGE); }
            }
            if (request.getNewAlbumName() != null && !request.getNewAlbumName().isBlank()) {
                if (request.getAlbumId() != null && request.getAlbumId() > 0) throw new BizException(ErrorCode.PARAM);
                Album album = albumService.resolveForUpload(null, request.getNewAlbumName(), request.getNewAlbumArtist(), cover == null ? null : cover.hash(), cover == null ? null : cover.format());
                song.setAlbumId(album.getId()); song.setAlbum(album.getName());
            } else if (request.getAlbumId() != null && request.getAlbumId() == 0) {
                if (cover != null || request.getNewAlbumArtist() != null && !request.getNewAlbumArtist().isBlank()) throw new BizException(ErrorCode.PARAM);
                song.setAlbumId(null); song.setAlbum(null);
            } else if (request.getAlbumId() != null && request.getAlbumId() > 0) {
                Album album = albumService.require(request.getAlbumId());
                if (request.getAlbumName() != null && !request.getAlbumName().isBlank() || request.getAlbumArtist() != null) {
                    album = albumService.updateMetadata(album.getId(), request.getAlbumName(), request.getAlbumArtist(), cover == null ? null : cover.hash(), cover == null ? null : cover.format());
                    mapper.updateAlbumName(album.getId(), album.getName());
                } else if (cover != null) {
                    album = albumService.updateMetadata(album.getId(), album.getName(), album.getArtist(), cover.hash(), cover.format());
                }
                song.setAlbumId(album.getId()); song.setAlbum(album.getName());
            } else if (cover != null) {
                throw new BizException(ErrorCode.PARAM);
            }
        }
        song.setTitle(request.getTitle().trim()); song.setArtist(blankToNull(request.getArtist()));
        if (request.getDurationMs() != null) song.setDurationMs(request.getDurationMs());
        mapper.updateById(song);
        return response(song);
    }
    @Override @org.springframework.transaction.annotation.Transactional(rollbackFor = Exception.class)
    public void delete(Long id) { require(id); mapper.deleteById(id); }
    public SongLyricsResponse lyrics(Long songId) {
        require(songId);
        return new SongLyricsResponse(songId, mapper.selectLyrics(songId));
    }
    public SongCoverResponse cover(Long songId) {
        Song song = require(songId);
        if (song.getAlbumId() != null) {
            Album album = albumService.require(song.getAlbumId());
            if (album.getCoverHash() != null) {
                var cover = albumService.cover(album.getId());
                return new SongCoverResponse(cover.getInternalPath(), cover.getContentType(), cover.getFilePath(), cover.getFileSize());
            }
        }
        if (song.getCoverHash() == null || song.getCoverFormat() == null) throw new BizException(ErrorCode.NOT_FOUND);
        String format = song.getCoverFormat();
        String mime = switch (format) { case "jpg" -> "image/jpeg"; case "png" -> "image/png"; case "webp" -> "image/webp"; default -> throw new BizException(ErrorCode.NOT_FOUND); };
        if (!song.getCoverHash().matches("[a-f0-9]{64}")) throw new BizException(ErrorCode.NOT_FOUND);
        Path directory = Path.of(storageDirectory).toAbsolutePath().normalize();
        Path file = directory.resolve("covers").resolve(song.getCoverHash() + "." + format).normalize();
        if (!file.startsWith(directory) || !Files.isRegularFile(file)) throw new BizException(ErrorCode.NOT_FOUND);
        try {
            return new SongCoverResponse("/protected-music/covers/" + song.getCoverHash() + "." + format,
                    mime, file.toString(), Files.size(file));
        } catch (java.io.IOException ex) {
            log.error("歌曲封面读取失败，song {}", song.getId(), ex);
            throw new BizException(ErrorCode.SONG_STORAGE);
        }
    }
    public SongFileResponse file(String hash) {
        Song song = requireHash(hash);
        Path directory = Path.of(storageDirectory).toAbsolutePath().normalize();
        Path file = directory.resolve(hash).normalize();
        if (!file.startsWith(directory) || !Files.isRegularFile(file)) throw new BizException(ErrorCode.NOT_FOUND);
        String contentType = switch (song.getFormat()) {
            case "mp3" -> "audio/mpeg";
            case "flac" -> "audio/flac";
            case "aac" -> "audio/aac";
            case "ogg" -> "audio/ogg";
            case "wav" -> "audio/wav";
            case "m4a" -> "audio/mp4";
            default -> "application/octet-stream";
        };
        try {
            return new SongFileResponse(file.toString(), contentType, Files.size(file));
        } catch (java.io.IOException ex) {
            log.error("歌曲文件读取失败，song {}", song.getId(), ex);
            throw new BizException(ErrorCode.SONG_STORAGE);
        }
    }
    private String readLyrics(SongUploadRequest request) {
        return readLyrics(request.getLyricsFile(), request.getLyricsText());
    }
    private String readLyrics(MultipartFile file, String lyricsText) {
        String text = blankToNull(lyricsText);
        if (file != null && text != null) throw new BizException(ErrorCode.PARAM);
        if (file == null) {
            if (text != null && text.getBytes(StandardCharsets.UTF_8).length > MAX_LYRICS_BYTES) throw new BizException(ErrorCode.SONG_LYRICS_SIZE);
            return text;
        }
        if (file.isEmpty() || !LYRIC_FORMATS.contains(extension(file))) throw new BizException(ErrorCode.SONG_LYRICS_FORMAT);
        if (file.getSize() > MAX_LYRICS_BYTES) throw new BizException(ErrorCode.SONG_LYRICS_SIZE);
        try {
            byte[] bytes = file.getBytes();
            if (bytes.length > MAX_LYRICS_BYTES) throw new BizException(ErrorCode.SONG_LYRICS_SIZE);
            String decoded = StandardCharsets.UTF_8.newDecoder().onMalformedInput(CodingErrorAction.REPORT).onUnmappableCharacter(CodingErrorAction.REPORT).decode(ByteBuffer.wrap(bytes)).toString();
            return blankToNull(decoded.startsWith("\uFEFF") ? decoded.substring(1) : decoded);
        } catch (CharacterCodingException ex) { throw new BizException(ErrorCode.SONG_LYRICS_FORMAT); }
        catch (java.io.IOException ex) { throw new BizException(ErrorCode.SONG_STORAGE); }
    }
    private AudioPayload storeAudio(MultipartFile file, Path directory) throws java.io.IOException, NoSuchAlgorithmException {
        Path temporary = Files.createTempFile(directory, "replace-", ".tmp");
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        try (InputStream input = file.getInputStream(); var output = Files.newOutputStream(temporary)) {
            byte[] buffer = new byte[8192]; int count;
            while ((count = input.read(buffer)) != -1) { digest.update(buffer, 0, count); output.write(buffer, 0, count); }
        } catch (java.io.IOException ex) { Files.deleteIfExists(temporary); throw ex; }
        return new AudioPayload(HexFormat.of().formatHex(digest.digest()), temporary);
    }
    private String extension(MultipartFile file) {
        String name = file.getOriginalFilename();
        int dot = name == null ? -1 : name.lastIndexOf('.');
        return dot < 0 ? "" : name.substring(dot + 1).toLowerCase(java.util.Locale.ROOT);
    }
    private String blankToNull(String value) { return value == null || value.isBlank() ? null : value; }
    private boolean invalidDuration(Integer durationMs) { return durationMs != null && (durationMs < 0 || durationMs > MAX_DURATION_MS); }
    private record AudioPayload(String hash, Path temporary) { }
}
