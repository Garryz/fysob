#ifndef ENGINE_HANDLER_DELIMITER_BASED_FRAME_DECODER_H
#define ENGINE_HANDLER_DELIMITER_BASED_FRAME_DECODER_H

#include <exception>
#include <initializer_list>
#include <vector>

#include <engine/common/any.h>
#include <engine/handler/abstract_handler.h>
#include <engine/net/asio_buffer.h>

namespace engine
{

class delimiter_based_frame_decoder : public abstract_handler
{
public:
    delimiter_based_frame_decoder(const delimiter_based_frame_decoder&) = delete;
    delimiter_based_frame_decoder operator=(const delimiter_based_frame_decoder&) = delete;

    delimiter_based_frame_decoder(uint32_t max_frame_length, 
                                    std::string delimiter,
                                    bool strip_delimiter = true)
        : max_frame_length_(max_frame_length)
        , strip_delimiter_(strip_delimiter)
    {
        validate_delimiter(delimiter);
        delimiters_.push_back(delimiter);
    }

    delimiter_based_frame_decoder(uint32_t max_frame_length,
                                    std::initializer_list<std::string> delimiters,
                                    bool strip_delimiter = true)
        : max_frame_length_(max_frame_length)
        , delimiters_(delimiters)
        , strip_delimiter_(strip_delimiter)
    {
        for (auto delimiter : delimiters) {
            validate_delimiter(delimiter);
        }
    }

    virtual void decode(context* ctx, std::unique_ptr<any> msg)
    {
        auto buffer = any_cast<std::shared_ptr<asio_buffer>>(*msg);
        assert(buffer);
        while (buffer->readable_bytes() > 0) {
            int min_frame_length = max_frame_length_ + 1;
            std::string min_delim = "";
            for (auto delim : delimiters_) {
                int frame_length = index_of(buffer, delim);
                if (frame_length >= 0 && frame_length < min_frame_length) {
                    min_frame_length = frame_length;
                    min_delim = delim;
                }
            }

            if (min_delim != "") {
                int min_delim_length = min_delim.size();

                if (min_frame_length > max_frame_length_) {
                    LOGF(WARNING, "min_frame_length = %d, max_frame_length = %d", 
                            min_frame_length, max_frame_length_);
                    buffer->retrieve(min_frame_length + min_delim_length);
                    return;
                }

                std::unique_ptr<data_block> data;
                if (strip_delimiter_) {
                    data = buffer->read(min_frame_length);
                    buffer->retrieve(min_delim_length);
                } else {
                    data = buffer->read(min_frame_length + min_delim_length);
                }
                auto response = new any(read_data(*data));
                ctx->fire_read(std::unique_ptr<any>(response));
            }
        }
    }

private:
    static void validate_delimiter(const std::string& delimiter) {
        if (delimiter == "") {
            throw std::invalid_argument("string is empty");
        }
    }

    static int index_of(std::shared_ptr<asio_buffer> haystack, const std::string& needle) {
        for (std::size_t i = 0; i < haystack->readable_bytes(); ++i) {
            std::size_t haystack_index = i;
            std::size_t needle_index;
            for (needle_index = 0; needle_index < needle.size(); ++needle_index) {
                if (haystack->peek_index_endian<char>(haystack_index, true) 
                        != needle[needle_index]) {
                    break;
                } else {
                    haystack_index++;
                    if (haystack_index == haystack->readable_bytes() &&
                            needle_index != needle.size() - 1) {
                        return -1;
                    }
                }
            }

            if (needle_index == needle.size()) {
                return i;
            }
        } 
        return -1; 
    }

    uint32_t                    max_frame_length_;
    std::vector<std::string>    delimiters_;
    bool                        strip_delimiter_;
}; // class delimiter_based_frame_decoder

} // namespace engine

#endif // ENGINE_HANDLER_DELIMITER_BASED_FRAME_DECODER_H



