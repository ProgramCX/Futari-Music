package com.chengxu.futari.playlist.server.service.impl;

import com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper;
import com.chengxu.futari.common.error.BizException;
import com.chengxu.futari.common.error.ErrorCode;
import com.chengxu.futari.playlist.personal.dto.AddSongsResponse;
import com.chengxu.futari.playlist.server.dto.*;
import com.chengxu.futari.playlist.server.entity.*;
import com.chengxu.futari.playlist.server.mapper.*;
import com.chengxu.futari.playlist.server.service.ServerPlaylistService;
import com.chengxu.futari.song.service.SongService;
import com.chengxu.futari.song.dto.SongResponse;
import java.time.ZoneId;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

/** 服务器公共歌单实现。 @author Futari */
@Service @RequiredArgsConstructor
public class ServerPlaylistServiceImpl implements ServerPlaylistService {
    private final ServerPlaylistMapper playlists;
    private final ServerPlaylistSongMapper entries;
    private final SongService songs;
    private ServerPlaylist require(Long id) {
        ServerPlaylist row = playlists.selectById(id);
        if (row == null) throw new BizException(ErrorCode.PLAYLIST_NOT_FOUND);
        return row;
    }
    private List<ServerPlaylistSong> entries(Long id) {
        return entries.selectList(new LambdaQueryWrapper<ServerPlaylistSong>().eq(ServerPlaylistSong::getPlaylistId, id).orderByAsc(ServerPlaylistSong::getSortOrder).orderByAsc(ServerPlaylistSong::getId));
    }
    private ServerPlaylistResponse response(ServerPlaylist row) {
        Long created = row.getCreatedAt() == null ? null : row.getCreatedAt().atZone(ZoneId.systemDefault()).toInstant().toEpochMilli();
        return new ServerPlaylistResponse(row.getId(), row.getName(), row.getDescription(), row.getCoverUrl(), row.getCreatorId(), created,
                songs.findAvailableByIds(entries(row.getId()).stream().map(ServerPlaylistSong::getSongId).toList()));
    }
    public List<ServerPlaylistResponse> list() { return playlists.selectList(new LambdaQueryWrapper<ServerPlaylist>().orderByDesc(ServerPlaylist::getId)).stream().map(this::response).toList(); }
    public ServerPlaylistResponse get(Long id) { return response(require(id)); }
    public ServerPlaylistResponse create(Long userId, ServerPlaylistCreateRequest request) {
        ServerPlaylist row = new ServerPlaylist(); row.setName(request.getName().trim()); row.setDescription(request.getDescription()); row.setCoverUrl(request.getCoverUrl()); row.setCreatorId(userId);
        playlists.insert(row); return response(row);
    }
    public ServerPlaylistResponse update(Long id, ServerPlaylistUpdateRequest request) {
        ServerPlaylist row = require(id); row.setName(request.getName().trim()); row.setDescription(request.getDescription()); row.setCoverUrl(request.getCoverUrl());
        playlists.updateById(row); return response(row);
    }
    public void delete(Long id) { require(id); playlists.deleteById(id); }
    @Transactional(rollbackFor = Exception.class)
    public AddSongsResponse addSongs(Long id, List<Long> songIds) {
        require(id);
        if (songIds == null || songIds.size() > 500 || songIds.stream().anyMatch(songId -> songId == null || songId < 1)) throw new BizException(ErrorCode.PARAM);
        int added = 0; int order = entries(id).size();
        for (Long songId : songIds) { songs.require(songId); added += entries.insertIgnore(id, songId, order++); }
        return new AddSongsResponse(added, songIds.size() - added);
    }
    public void removeSong(Long id, Long songId) { require(id); entries.removeSong(id, songId); }
    @Transactional(rollbackFor = Exception.class)
    public void reorder(Long id, List<Long> songIds) {
        require(id);
        if (songIds == null || songIds.size() > 500) throw new BizException(ErrorCode.PARAM);
        Set<Long> present = new HashSet<>(songs.findAvailableByIds(entries(id).stream().map(ServerPlaylistSong::getSongId).toList())
                .stream().map(SongResponse::getId).toList());
        if (present.size() != songIds.size() || !present.equals(new HashSet<>(songIds))) throw new BizException(ErrorCode.PLAYLIST_ORDER);
        for (int i = 0; i < songIds.size(); i++) entries.reorder(id, songIds.get(i), i);
    }
}
