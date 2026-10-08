package com.chengxu.futari.user.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.chengxu.futari.user.entity.User;
import org.apache.ibatis.annotations.Mapper;

/** 用户数据访问。 @author Futari */
@Mapper
public interface UserMapper extends BaseMapper<User> { }
