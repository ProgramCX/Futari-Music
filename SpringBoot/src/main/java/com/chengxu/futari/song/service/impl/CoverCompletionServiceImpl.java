package com.chengxu.futari.song.service.impl;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.baomidou.mybatisplus.core.conditions.update.LambdaUpdateWrapper;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.song.dto.*;
import com.chengxu.futari.song.entity.Album;
import com.chengxu.futari.song.entity.Song;
import com.chengxu.futari.song.mapper.AlbumMapper;
import com.chengxu.futari.song.mapper.SongMapper;
import com.chengxu.futari.song.service.CoverCompletionService;
import java.nio.file.Path;
import java.io.IOException;
import com.chengxu.futari.common.result.PageResult;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

/** 使用条件更新保证预览后并发补全不会覆盖已有封面。 @author Futari */
@Service @RequiredArgsConstructor @Slf4j
public class CoverCompletionServiceImpl implements CoverCompletionService {
    private final SongMapper songs;
    private final AlbumMapper albums;
    @Value("${futari.storage.directory}") private String storageDirectory;

    public PageResult<CoverCandidateResponse> songs(int pageNum, int pageSize) {
        validatePage(pageNum, pageSize);
        var page = songs.selectCoverCandidates(new Page<>(pageNum, pageSize));
        return new PageResult<>(page.getTotal(), page.getRecords());
    }

    public PageResult<AlbumResponse> albums(int pageNum, int pageSize) {
        validatePage(pageNum, pageSize);
        var query = new LambdaQueryWrapper<Album>()
                .and(q -> q.isNull(Album::getCoverHash).or().eq(Album::getCoverHash, ""))
                .orderByAsc(Album::getId);
        var page = albums.selectPage(new Page<Album>(pageNum, pageSize), query);
        return new PageResult<>(page.getTotal(), page.getRecords().stream()
                .map(a -> new AlbumResponse(a.getId(), a.getName(), a.getArtist(), null)).toList());
    }

    public CoverCompletionResponse completeSong(Long id, CoverCompletionRequest request) {
        Song song = songs.selectById(id);
        if (song == null) throw new BizException(ErrorCode.NOT_FOUND);
        if (song.getCoverHash() != null && !song.getCoverHash().isBlank()) return new CoverCompletionResponse(false);
        var cover = store(request);
        int changed = songs.update(null, new LambdaUpdateWrapper<Song>().eq(Song::getId, id)
                .and(q -> q.isNull(Song::getCoverHash).or().eq(Song::getCoverHash, ""))
                .set(Song::getCoverHash, cover.hash()).set(Song::getCoverFormat, cover.format()));
        return new CoverCompletionResponse(changed > 0);
    }

    public CoverCompletionResponse completeAlbum(Long id, CoverCompletionRequest request) {
        Album album = albums.selectById(id);
        if (album == null) throw new BizException(ErrorCode.NOT_FOUND);
        if (album.getCoverHash() != null && !album.getCoverHash().isBlank()) return new CoverCompletionResponse(false);
        var cover = store(request);
        int changed = albums.update(null, new LambdaUpdateWrapper<Album>().eq(Album::getId, id)
                .and(q -> q.isNull(Album::getCoverHash).or().eq(Album::getCoverHash, ""))
                .set(Album::getCoverHash, cover.hash()).set(Album::getCoverFormat, cover.format()));
        return new CoverCompletionResponse(changed > 0);
    }

    private CoverFiles.CoverPayload store(CoverCompletionRequest request) {
        var cover = CoverFiles.readCover(request.getFile());
        if (cover == null) throw new BizException(ErrorCode.SONG_EMPTY);
        try {
            CoverFiles.storeCover(Path.of(storageDirectory).toAbsolutePath().normalize(), cover);
            return cover;
        } catch (IOException ex) {
            log.error("补全封面文件存储失败", ex);
            throw new BizException(ErrorCode.SONG_STORAGE);
        }
    }

    private void validatePage(int pageNum, int pageSize) {
        if (pageNum < 1 || pageSize < 1 || pageSize > 100)
            throw new BizException(ErrorCode.PARAM);
    }
}
