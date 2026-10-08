package com.chengxu.futari.song.service.impl;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.song.dto.AlbumCoverResponse;
import com.chengxu.futari.song.dto.AlbumResponse;
import com.chengxu.futari.song.entity.Album;
import com.chengxu.futari.song.mapper.AlbumMapper;
import com.chengxu.futari.song.service.AlbumService;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Collection;
import java.util.List;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.dao.DuplicateKeyException;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

/** 专辑共享元数据和专辑封面存储。 @author Futari */
@Slf4j @Service @RequiredArgsConstructor
public class AlbumServiceImpl implements AlbumService {
    private final AlbumMapper mapper;
    @Value("${futari.storage.directory}") private String storageDirectory;

    @Override public PageResult<AlbumResponse> list(String keyword, int pageNum, int pageSize) {
        if (pageNum < 1 || pageSize < 1 || pageSize > 100) throw new BizException(ErrorCode.PARAM);
        String value = keyword == null ? "" : keyword.trim();
        Page<Album> page = mapper.selectPage(new Page<>(pageNum, pageSize),
                new LambdaQueryWrapper<Album>().and(!value.isBlank(), query -> query.like(Album::getName, value).or().like(Album::getArtist, value))
                        .orderByAsc(Album::getName).orderByAsc(Album::getArtist));
        List<AlbumResponse> rows = page.getRecords().stream().map(album -> new AlbumResponse(
                album.getId(), album.getName(), album.getArtist(), album.getCoverHash() == null ? null : "/api/albums/" + album.getId() + "/cover")).toList();
        return new PageResult<>(page.getTotal(), rows);
    }

    @Override public Album require(Long id) {
        Album album = id == null ? null : mapper.selectById(id);
        if (album == null) throw new BizException(ErrorCode.NOT_FOUND);
        return album;
    }

    @Override public List<Album> findByName(String name) {
        if (name == null || name.isBlank()) return List.of();
        return mapper.selectList(new LambdaQueryWrapper<Album>().eq(Album::getName, name.trim())
                .orderByAsc(Album::getArtist).orderByAsc(Album::getId));
    }

    @Override public Album findByIdentity(String name, String artist) {
        if (name == null || name.isBlank()) return null;
        String cleanedArtist = artist == null ? "" : artist.trim();
        return mapper.selectOne(new LambdaQueryWrapper<Album>().eq(Album::getName, name.trim())
                .eq(Album::getArtist, cleanedArtist).last("LIMIT 1"));
    }

    @Override public List<Album> findByIds(Collection<Long> ids) {
        return ids == null || ids.isEmpty() ? List.of() : mapper.selectBatchIds(ids);
    }

    @Override @Transactional(rollbackFor = Exception.class)
    public Album resolveForUpload(Long albumId, String name, String artist, String coverHash, String coverFormat) {
        if (albumId != null && albumId > 0) {
            if (name != null && !name.isBlank() || artist != null && !artist.isBlank()) throw new BizException(ErrorCode.PARAM);
            Album existing = require(albumId);
            if (coverHash != null) {
                existing.setCoverHash(coverHash); existing.setCoverFormat(coverFormat); mapper.updateById(existing);
            }
            return existing;
        }
        String cleaned = name == null ? "" : name.trim();
        String cleanedArtist = artist == null ? "" : artist.trim();
        if (cleaned.isEmpty()) {
            if (!cleanedArtist.isEmpty() || coverHash != null) throw new BizException(ErrorCode.PARAM);
            return null;
        }
        if (cleaned.length() > 128 || cleanedArtist.length() > 128) throw new BizException(ErrorCode.PARAM);
        List<Album> sameName = findByName(cleaned);
        if (cleanedArtist.isEmpty() && sameName.size() > 1) throw new BizException(ErrorCode.ALBUM_AMBIGUOUS);
        Album existing = cleanedArtist.isEmpty() && sameName.size() == 1 ? sameName.get(0) : findByIdentity(cleaned, cleanedArtist);
        if (existing != null) {
            if (coverHash != null) {
                existing.setCoverHash(coverHash); existing.setCoverFormat(coverFormat); mapper.updateById(existing);
            }
            return existing;
        }
        Album album = new Album(); album.setName(cleaned); album.setArtist(cleanedArtist); album.setCoverHash(coverHash); album.setCoverFormat(coverFormat);
        try { mapper.insert(album); }
        catch (DuplicateKeyException ex) {
            Album raced = findByIdentity(cleaned, cleanedArtist);
            if (raced == null) throw new BizException(ErrorCode.ALBUM_EXISTS);
            return raced;
        }
        return album;
    }

    @Override @Transactional(rollbackFor = Exception.class)
    public Album updateMetadata(Long id, String name, String artist, String coverHash, String coverFormat) {
        Album album = require(id);
        String cleaned = name == null ? album.getName() : name.trim();
        String cleanedArtist = artist == null ? album.getArtist() : artist.trim();
        if (cleaned.isEmpty() || cleaned.length() > 128 || cleanedArtist.length() > 128) throw new BizException(ErrorCode.PARAM);
        Album duplicate = findByIdentity(cleaned, cleanedArtist);
        if (duplicate != null && !duplicate.getId().equals(id)) throw new BizException(ErrorCode.ALBUM_EXISTS);
        album.setName(cleaned);
        album.setArtist(cleanedArtist);
        if (coverHash != null) { album.setCoverHash(coverHash); album.setCoverFormat(coverFormat); }
        try { mapper.updateById(album); }
        catch (DuplicateKeyException ex) { throw new BizException(ErrorCode.ALBUM_EXISTS); }
        return album;
    }

    @Override public AlbumCoverResponse cover(Long id) {
        Album album = require(id);
        String hash = album.getCoverHash();
        String format = album.getCoverFormat();
        if (hash == null || !hash.matches("[a-f0-9]{64}") || format == null) throw new BizException(ErrorCode.NOT_FOUND);
        String mime = switch (format) { case "jpg" -> "image/jpeg"; case "png" -> "image/png"; case "webp" -> "image/webp"; default -> throw new BizException(ErrorCode.NOT_FOUND); };
        Path directory = Path.of(storageDirectory).toAbsolutePath().normalize();
        Path file = directory.resolve("covers").resolve(hash + "." + format).normalize();
        if (!file.startsWith(directory) || !Files.isRegularFile(file)) throw new BizException(ErrorCode.NOT_FOUND);
        try { return new AlbumCoverResponse("/protected-music/covers/" + hash + "." + format, mime, file.toString(), Files.size(file)); }
        catch (IOException ex) { log.error("专辑封面读取失败，album {}", id, ex); throw new BizException(ErrorCode.SONG_STORAGE); }
    }
}
