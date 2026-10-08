package com.chengxu.futari.playlist.server.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.chengxu.futari.playlist.server.entity.ServerPlaylist;
import org.apache.ibatis.annotations.Mapper;

/** 公共歌单数据访问。 @author Futari */
@Mapper public interface ServerPlaylistMapper extends BaseMapper<ServerPlaylist> { }
