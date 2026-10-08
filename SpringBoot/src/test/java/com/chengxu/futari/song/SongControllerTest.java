package com.chengxu.futari.song;

import com.chengxu.futari.song.controller.SongController;
import com.chengxu.futari.song.dto.SongFileResponse;
import com.chengxu.futari.song.service.SongService;
import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import org.springframework.test.web.servlet.MockMvc;
import org.springframework.test.web.servlet.setup.MockMvcBuilders;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.content;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.header;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

/** 歌曲下载在直连 Spring 与 Nginx 代理下都能正确响应。 @author Futari */
class SongControllerTest {
    @TempDir Path storage;

    @Test void directSpringDownloadSupportsByteRanges() throws Exception {
        String hash = "a".repeat(64);
        byte[] bytes = {0, 1, 2, 3, 4};
        Path audio = storage.resolve(hash);
        Files.write(audio, bytes);
        SongService service = mock(SongService.class);
        when(service.file(hash)).thenReturn(new SongFileResponse(audio.toString(), "audio/mpeg", bytes.length));
        MockMvc mvc = MockMvcBuilders.standaloneSetup(new SongController(service)).build();

        mvc.perform(get("/api/songs/{hash}/file", hash).header("Range", "bytes=1-3"))
                .andExpect(status().isPartialContent())
                .andExpect(header().string("Content-Range", "bytes 1-3/5"))
                .andExpect(content().bytes(new byte[]{1, 2, 3}));
    }

    @Test void nginxProxyKeepsInternalFileRedirect() throws Exception {
        String hash = "b".repeat(64);
        SongService service = mock(SongService.class);
        when(service.file(hash)).thenReturn(new SongFileResponse(storage.resolve(hash).toString(), "audio/mpeg", 5));
        MockMvc mvc = MockMvcBuilders.standaloneSetup(new SongController(service)).build();

        mvc.perform(get("/api/songs/{hash}/file", hash).header("X-Futari-Nginx", "1"))
                .andExpect(status().isOk())
                .andExpect(header().string("X-Accel-Redirect", "/protected-music/" + hash))
                .andExpect(content().string(""));
    }
}
