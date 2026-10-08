package com.chengxu.futari.playlist.server.service;

import com.chengxu.futari.playlist.personal.dto.AddSongsResponse;
import com.chengxu.futari.playlist.server.dto.*;
import java.util.List;

/** 公共歌单业务。 @author Futari */
public interface ServerPlaylistService {
    List<ServerPlaylistResponse> list();
    ServerPlaylistResponse get(Long id);
    ServerPlaylistResponse create(Long userId, ServerPlaylistCreateRequest request);
    ServerPlaylistResponse update(Long id, ServerPlaylistUpdateRequest request);
    void delete(Long id);
    AddSongsResponse addSongs(Long id, List<Long> songIds);
    void removeSong(Long id, Long songId);
    void reorder(Long id, List<Long> songIds);
}
