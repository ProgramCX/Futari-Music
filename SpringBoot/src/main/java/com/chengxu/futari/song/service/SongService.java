package com.chengxu.futari.song.service;

import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.song.dto.SongResponse;
import com.chengxu.futari.song.dto.SongLyricsResponse;
import com.chengxu.futari.song.dto.SongUploadRequest;
import com.chengxu.futari.song.dto.SongCoverResponse;
import com.chengxu.futari.song.dto.SongFileResponse;
import com.chengxu.futari.song.dto.SongUpdateRequest;
import com.chengxu.futari.song.entity.Song;
import java.util.List;

/** 音乐元数据、上传和受控下载。 @author Futari */
public interface SongService {
    Song require(Long id);
    SongResponse response(Song song);
    List<SongResponse> findAvailableByIds(List<Long> songIds);
    PageResult<SongResponse> list(String keyword, int pageNum, int pageSize);
    PageResult<SongResponse> listMine(Long userId, int pageNum, int pageSize);
    SongResponse upload(Long userId, SongUploadRequest request);
    SongResponse update(Long id, SongUpdateRequest request);
    void delete(Long id);
    Song requireHash(String hash);
    SongLyricsResponse lyrics(Long songId);
    SongCoverResponse cover(Long songId);
    SongFileResponse file(String hash);
}
