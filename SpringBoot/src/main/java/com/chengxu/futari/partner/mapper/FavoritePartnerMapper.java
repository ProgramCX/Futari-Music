package com.chengxu.futari.partner.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.chengxu.futari.partner.entity.FavoritePartner;
import java.util.List;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Delete;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Select;

/** 搭子收藏数据访问。 @author Futari */
@Mapper
public interface FavoritePartnerMapper extends BaseMapper<FavoritePartner> {
    @Delete("DELETE FROM favorite_partner WHERE user_id = #{userId} AND partner_id = #{partnerId}")
    int remove(@Param("userId") Long userId, @Param("partnerId") Long partnerId);
    @Select("SELECT user_id FROM favorite_partner WHERE partner_id = #{partnerId} AND deleted = 0")
    List<Long> ownersOf(@Param("partnerId") Long partnerId);
}
