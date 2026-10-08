package com.chengxu.futari.playlist.personal.service;

import com.chengxu.futari.playlist.personal.dto.*;
import java.util.List;

/** 个人歌单业务。 @author Futari */
public interface UserPlaylistService {
    List<PlaylistResponse> list(Long userId);
    PlaylistResponse get(Long userId, Long id);
    PlaylistResponse create(Long userId, PlaylistCreateRequest request);
    PlaylistResponse update(Long userId, Long id, PlaylistUpdateRequest request);
    void delete(Long userId, Long id);
    AddSongsResponse addSongs(Long userId, Long id, List<Long> songIds);
    void removeSong(Long userId, Long id, Long songId);
    void reorder(Long userId, Long id, List<Long> songIds);
}
