package com.chengxu.futari.playlist.personal.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.chengxu.futari.playlist.personal.entity.UserPlaylist;
import org.apache.ibatis.annotations.Mapper;

/** 个人歌单数据访问。 @author Futari */
@Mapper public interface UserPlaylistMapper extends BaseMapper<UserPlaylist> { }
