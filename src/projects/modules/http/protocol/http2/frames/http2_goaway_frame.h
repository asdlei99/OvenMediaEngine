//==============================================================================
//
//  OvenMediaEngine
//
//  Created by Getroot
//  Copyright (c) 2022 AirenSoft. All rights reserved.
//
//==============================================================================
#pragma once

#include <base/ovlibrary/ovlibrary.h>
#include <base/ovlibrary/byte_io.h>
#include "../http2_frame.h"

namespace http
{
	namespace prot
	{
		namespace h2
		{
			class Http2GoAwayFrame : public Http2Frame
			{
			public:
				// Make from parsed general frame header and payload
				Http2GoAwayFrame(const std::shared_ptr<const Http2Frame> &frame)
					: Http2Frame(frame)
				{
					ParsingPayload();
				}

				// Make by itself
				Http2GoAwayFrame()
					// https://datatracker.ietf.org/doc/html/rfc7540#section-6.5
					// SETTINGS frames always apply to a connection, never a single stream. 
					// The stream identifier for a SETTINGS frame MUST be zero (0x00).
					: Http2Frame(0)
				{
					SetType(Http2Frame::Type::GoAway);
					// Settings Frame's stream id is 0
					SetStreamId(0);
				}

				// Setters
				void SetLastStreamId(uint32_t last_stream_id)
				{
					_last_stream_id = last_stream_id;
				}

				void SetErrorCode(uint32_t error_code)
				{
					_error_code = error_code;
				}

				// Getters
				uint32_t GetLastStreamId() const
				{
					return _last_stream_id;
				}

				uint32_t GetErrorCode() const
				{
					return _error_code;
				}

				// To String
				ov::String ToString() const override
				{
					ov::String str;
					
					str = Http2Frame::ToString();

					str += "\n";
					str += "[GoAway Frame]\n";

					// Print parameters
					str += ov::String::FormatString("Last-Stream-ID : %u\n", _last_stream_id);
					str += ov::String::FormatString("Error Code : %u\n", _error_code);
					
					if (_additional_debug_data != nullptr && _additional_debug_data->GetLength() > 0)
					{
						str += ov::String::FormatString("Additional Debug Data[Str] : %s\n", _additional_debug_data->ToString().CStr());
						str += ov::String::FormatString("Additional Debug Data[Hex] : %s\n", _additional_debug_data->ToHexString().CStr());
					}

					return str;
				}

				// To data
				const std::shared_ptr<const ov::Data> GetPayload() const override
				{
					if (Http2Frame::GetPayload() != nullptr)
					{
						return Http2Frame::GetPayload();
					}

					auto payload = std::make_shared<ov::Data>(8);
					ov::ByteStream stream(payload.get()); 

					stream.WriteBE32(_last_stream_id);
					stream.WriteBE32(_error_code);

					return payload;
				}

			private:
				void ParsingPayload()
				{
					// The payload of a SETTINGS frame consists of zero or more parameters,
					auto payload = GetPayload();
					if (payload == nullptr)
					{
						SetParsingState(ParsingState::Completed);
						return;
					}
					
					// Parse each parameter in the payload
					auto payload_data = payload->GetDataAs<uint8_t>();
					auto payload_size = payload->GetLength();
					size_t payload_offset = 0;

					auto _last_stream_id = ByteReader<uint32_t>::ReadBigEndian(payload_data + payload_offset);
					payload_offset += sizeof(uint32_t);

					// unset first bit (reserved)
					_last_stream_id &= 0x7FFFFFFF;

					_error_code = ByteReader<uint32_t>::ReadBigEndian(payload_data + payload_offset);
					payload_offset += sizeof(uint32_t);

					// Additional debugging information if data remains.
					if (payload_offset < payload_size)
					{
						_additional_debug_data = std::make_shared<ov::Data>(payload_data + payload_offset, payload_size - payload_offset);
					}

					SetParsingState(ParsingState::Completed);
				}

				uint32_t _last_stream_id = 0;
				uint32_t _error_code = 0;
				std::shared_ptr<ov::Data> _additional_debug_data = nullptr;
			};
		}
	}
}