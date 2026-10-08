package com.chengxu.futari.song.service;

import com.chengxu.futari.common.result.PageResult;
import com.chengxu.futari.song.dto.AlbumCoverResponse;
import com.chengxu.futari.song.dto.AlbumResponse;
import com.chengxu.futari.song.entity.Album;
import java.util.Collection;
import java.util.List;

/** 管理歌曲共享的专辑实体。 @author Futari */
public interface AlbumService {
    PageResult<AlbumResponse> list(String keyword, int pageNum, int pageSize);
    Album require(Long id);
    List<Album> findByName(String name);
    Album findByIdentity(String name, String artist);
    List<Album> findByIds(Collection<Long> ids);
    Album resolveForUpload(Long albumId, String name, String artist, String coverHash, String coverFormat);
    Album updateMetadata(Long id, String name, String artist, String coverHash, String coverFormat);
    AlbumCoverResponse cover(Long id);
}
