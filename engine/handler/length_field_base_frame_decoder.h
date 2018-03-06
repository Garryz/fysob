#ifndef ENGINE_HANDLER_LENGTH_FIELD_BASE_FRAME_DECODER_H
#define ENGINE_HANDLER_LENGTH_FIELD_BASE_FRAME_DECODER_H


#include <engine/common/any.h>
#include <engine/handler/abstract_handler.h>
#include <engine/net/asio_buffer.h>

namespace engine
{

/**
 * <pre>
 * <b>lengthFieldOffset</b>   = <b>0</b>
 * <b>lengthFieldLength</b>   = <b>2</b>
 * lengthAdjustment    = 0
 * initialBytesToStrip = 0 (= do not strip header)
 *
 * BEFORE DECODE (14 bytes)         AFTER DECODE (14 bytes)
 * +--------+----------------+      +--------+----------------+
 * | Length | Actual Content |----->| Length | Actual Content |
 * | 0x000C | "HELLO, WORLD" |      | 0x000C | "HELLO, WORLD" |
 * +--------+----------------+      +--------+----------------+
 * </pre>
 *
 * <pre>
 * lengthFieldOffset   = 0
 * lengthFieldLength   = 2
 * lengthAdjustment    = 0
 * <b>initialBytesToStrip</b> = <b>2</b> (= the length of the Length field)
 *
 * BEFORE DECODE (14 bytes)         AFTER DECODE (12 bytes)
 * +--------+----------------+      +----------------+
 * | Length | Actual Content |----->| Actual Content |
 * | 0x000C | "HELLO, WORLD" |      | "HELLO, WORLD" |
 * +--------+----------------+      +----------------+
 * </pre>
 *
 * <pre>
 * lengthFieldOffset   =  0
 * lengthFieldLength   =  2
 * <b>lengthAdjustment</b>    = <b>-2</b> (= the length of the Length field)
 * initialBytesToStrip =  0
 *
 * BEFORE DECODE (14 bytes)         AFTER DECODE (14 bytes)
 * +--------+----------------+      +--------+----------------+
 * | Length | Actual Content |----->| Length | Actual Content |
 * | 0x000E | "HELLO, WORLD" |      | 0x000E | "HELLO, WORLD" |
 * +--------+----------------+      +--------+----------------+
 * </pre>
 *
 * <pre>
 * <b>lengthFieldOffset</b>   = <b>2</b> (= the length of Header 1)
 * <b>lengthFieldLength</b>   = <b>2</b>
 * lengthAdjustment    = 0
 * initialBytesToStrip = 0
 *
 * BEFORE DECODE (16 bytes)                      AFTER DECODE (16 bytes)
 * +----------+--------+----------------+      +----------+--------+----------------+
 * | Header 1 | Length | Actual Content |----->| Header 1 | Length | Actual Content |
 * |  0xCAFE  | 0x000C | "HELLO, WORLD" |      |  0xCAFE  | 0x000C | "HELLO, WORLD" |
 * +----------+--------+----------------+      +----------+--------+----------------+
 * </pre>
 *
 * <pre>
 * lengthFieldOffset   = 0
 * lengthFieldLength   = 2
 * <b>lengthAdjustment</b>    = <b>2</b> (= the length of Header 1)
 * initialBytesToStrip = 0
 *
 * BEFORE DECODE (16 bytes)                      AFTER DECODE (16 bytes)
 * +--------+----------+----------------+      +--------+----------+----------------+
 * | Length | Header 1 | Actual Content |----->| Length | Header 1 | Actual Content |
 * | 0x000C |  0xCAFE  | "HELLO, WORLD" |      | 0x000C |  0xCAFE  | "HELLO, WORLD" |
 * +--------+----------+----------------+      +--------+----------+----------------+
 * </pre>
 *
 * <pre>
 * lengthFieldOffset   = 1 (= the length of HDR1)
 * lengthFieldLength   = 2
 * <b>lengthAdjustment</b>    = <b>1</b> (= the length of HDR2)
 * <b>initialBytesToStrip</b> = <b>3</b> (= the length of HDR1 + LEN)
 *
 * BEFORE DECODE (16 bytes)                       AFTER DECODE (13 bytes)
 * +------+--------+------+----------------+      +------+----------------+
 * | HDR1 | Length | HDR2 | Actual Content |----->| HDR2 | Actual Content |
 * | 0xCA | 0x000C | 0xFE | "HELLO, WORLD" |      | 0xFE | "HELLO, WORLD" |
 * +------+--------+------+----------------+      +------+----------------+
 * </pre>
 *
 * <pre>
 * lengthFieldOffset   =  1
 * lengthFieldLength   =  2
 * <b>lengthAdjustment</b>    = <b>-3</b> (= the length of HDR1 + LEN, negative)
 * <b>initialBytesToStrip</b> = <b> 3</b>
 *
 * BEFORE DECODE (16 bytes)                       AFTER DECODE (13 bytes)
 * +------+--------+------+----------------+      +------+----------------+
 * | HDR1 | Length | HDR2 | Actual Content |----->| HDR2 | Actual Content |
 * | 0xCA | 0x0010 | 0xFE | "HELLO, WORLD" |      | 0xFE | "HELLO, WORLD" |
 * +------+--------+------+----------------+      +------+----------------+
 * </pre> 
 */
class length_field_base_frame_decoder : public abstract_handler
{
public:
    length_field_base_frame_decoder(const length_field_base_frame_decoder&) = delete;
    length_field_base_frame_decoder operator=(const length_field_base_frame_decoder&) = delete;

    length_field_base_frame_decoder(uint32_t max_frame_length,
                                    uint32_t length_field_offset,
                                    uint32_t length_field_length,
                                    int32_t length_adjustment = 0,
                                    uint32_t initial_bytes_to_strip = 0,
                                    bool big_endian = true)
        : max_frame_length_(max_frame_length)
        , length_field_offset_(length_field_offset)
        , length_field_length_(length_field_length)
        , length_adjustment_(length_adjustment)
        , initial_bytes_to_strip_(initial_bytes_to_strip)
        , big_endian_(big_endian)
    {
        length_field_end_offset_ = length_field_offset + length_field_length;
    }

    virtual void decode(context* ctx, std::unique_ptr<any> msg)
    {
        auto buffer = any_cast<std::shared_ptr<asio_buffer>>(*msg); 
        assert(buffer);
        while (buffer->readable_bytes() > 0) {
            if (buffer->readable_bytes() <= length_field_end_offset_) {
                return;
            }

            uint64_t frame_length = get_unajust_frame_length(buffer, 
                    length_field_offset_, length_field_length_, big_endian_);

            if (length_adjustment_ < 0 &&
                    abs(length_adjustment_) > length_field_end_offset_) {
                LOGF(FATAL, "no enough length to adjsut, length_adjustment_ = %d, length_field_end_offset_ = %d",
                        length_adjustment_, length_field_end_offset_);
            }
            frame_length += length_adjustment_ + length_field_end_offset_;
            
            if (frame_length < length_field_end_offset_) {
                LOGF(WARNING, "Adjust frame length (%d) is less than length field end offset: %d",
                        frame_length, length_field_end_offset_);
                buffer->retrieve(length_field_end_offset_);
                return;
            }

            if (frame_length > max_frame_length_) {
                LOGF(WARNING, "frame_length = %d, max_frame_length = %d",
                        frame_length, max_frame_length_);
                return;
            }

            uint32_t frame_length_int = static_cast<uint32_t>(frame_length); 
            if (buffer->readable_bytes() < frame_length_int) {
                LOGF(WARNING, "readable_bytes = %d, frame_length_int = %d",
                        buffer->readable_bytes(), frame_length_int);
                return;
            }

            if (initial_bytes_to_strip_ > frame_length_int) {
                LOGF(WARNING, "Adjusted frame length (%d) is less than initial bytes to strip: %d",
                        frame_length_int, initial_bytes_to_strip_);
                buffer->retrieve(frame_length_int);
                return;
            }

            buffer->retrieve(initial_bytes_to_strip_); 

            uint32_t actual_frame_length = frame_length_int - initial_bytes_to_strip_; 
            std::unique_ptr<data_block> data = buffer->read(actual_frame_length);
            auto response = new any(read_data(*data));
            ctx->fire_read(std::unique_ptr<any>(response));
        }
    }
private:
    uint64_t get_unajust_frame_length(std::shared_ptr<asio_buffer> buf, 
            uint32_t offset, uint32_t length, bool big_endian)
    {
        uint64_t frame_length = 0;
        switch(length) {
            case 1:
                frame_length = buf->peek_index_endian<uint8_t>(
                        offset, big_endian);
                break;
            case 2:
                frame_length = buf->peek_index_endian<uint16_t>(
                        offset, big_endian);
                break;
            case 4:
                frame_length = buf->peek_index_endian<uint32_t>(
                        offset, big_endian);
                break;
            case 8:
                frame_length = buf->peek_index_endian<uint64_t>(
                        offset, big_endian);
                break;
            default:
                LOGF(FATAL, "unsupported length field length: %d (expected: 1, 2, 4 or 8)", 
                        length);
        }
        return frame_length;
    }

    uint32_t    max_frame_length_;
    uint32_t    length_field_offset_;
    uint32_t    length_field_length_;
    int32_t     length_adjustment_;
    uint32_t    length_field_end_offset_;
    uint32_t    initial_bytes_to_strip_;
    bool        big_endian_;
}; // class length_field_base_frame_decoder

} // namespace

#endif // ENGINE_HANDLER_LENGTH_FIELD_BASE_FRAME_DECODER_H
