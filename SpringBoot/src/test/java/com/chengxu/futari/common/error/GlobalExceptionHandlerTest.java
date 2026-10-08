package com.chengxu.futari.common.error;

import org.junit.jupiter.api.Test;
import org.springframework.http.HttpStatus;
import org.springframework.web.multipart.MaxUploadSizeExceededException;

import static org.junit.jupiter.api.Assertions.assertEquals;

class GlobalExceptionHandlerTest {
    @Test void oversizedMultipartRequestReturnsReadablePayloadTooLargeResponse() {
        var response = new GlobalExceptionHandler().uploadTooLarge(new MaxUploadSizeExceededException(80L * 1024 * 1024));

        assertEquals(HttpStatus.PAYLOAD_TOO_LARGE, response.getStatusCode());
        assertEquals(ErrorCode.SONG_UPLOAD_SIZE.getCode(), response.getBody().getCode());
        assertEquals(ErrorCode.SONG_UPLOAD_SIZE.getMessage(), response.getBody().getMessage());
    }
}
