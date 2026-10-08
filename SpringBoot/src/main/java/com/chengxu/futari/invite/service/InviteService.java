package com.chengxu.futari.invite.service;

import com.chengxu.futari.invite.dto.InviteResponse;
import com.chengxu.futari.room.dto.RoomStateResponse;
import java.util.List;

/** 邀请创建与响应。 @author Futari */
public interface InviteService {
    InviteResponse create(Long fromId, Long roomId, Long targetId);
    RoomStateResponse accept(Long userId, String inviteId);
    void decline(Long userId, String inviteId);
    List<InviteResponse> pending(Long userId);
}
