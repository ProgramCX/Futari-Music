package com.chengxu.futari.song.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.chengxu.futari.song.entity.Album;
import org.apache.ibatis.annotations.Mapper;

/** 专辑数据访问。 @author Futari */
@Mapper public interface AlbumMapper extends BaseMapper<Album> { }
