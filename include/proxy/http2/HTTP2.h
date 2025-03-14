/** @file
 *
 *  Fundamental HTTP/2 protocol definitions and parsers.
 *
 *  @section license License
 *
 *  Licensed to the Apache Software Foundation (ASF) under one
 *  or more contributor license agreements.  See the NOTICE file
 *  distributed with this work for additional information
 *  regarding copyright ownership.  The ASF licenses this file
 *  to you under the Apache License, Version 2.0 (the
 *  "License"); you may not use this file except in compliance
 *  with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#pragma once

#include "tscore/ink_defs.h"
#include "tscore/ink_memory.h"
#include "proxy/http2/HPACK.h"
#include "proxy/hdrs/MIME.h"
#include "records/RecDefs.h"

#include "tsutil/Metrics.h"

using ts::Metrics;

class HTTPHdr;

// [RFC 9113] 5.1.1 Stream identifiers.
using Http2StreamId = uint32_t;

constexpr Http2StreamId HTTP2_CONNECTION_CONTROL_STREAM = 0;
constexpr uint8_t       HTTP2_FRAME_NO_FLAG             = 0;

// [RFC 7540] 6.9.2. Initial Flow Control Window Size
// the flow control window can be come negative so we need to track it with a signed type.
using Http2WindowSize = int32_t;

extern const char *const HTTP2_CONNECTION_PREFACE;
const size_t             HTTP2_CONNECTION_PREFACE_LEN = 24;

const size_t HTTP2_FRAME_HEADER_LEN       = 9;
const size_t HTTP2_DATA_PADLEN_LEN        = 1;
const size_t HTTP2_HEADERS_PADLEN_LEN     = 1;
const size_t HTTP2_PRIORITY_LEN           = 5;
const size_t HTTP2_RST_STREAM_LEN         = 4;
const size_t HTTP2_PING_LEN               = 8;
const size_t HTTP2_GOAWAY_LEN             = 8;
const size_t HTTP2_WINDOW_UPDATE_LEN      = 4;
const size_t HTTP2_SETTINGS_PARAMETER_LEN = 6;

// SETTINGS initial values. NOTE: These should not be modified
// unless the protocol changes! Do not change this thinking you
// are changing server defaults. that is done via RecordsConfig.cc
const uint32_t HTTP2_ENABLE_PUSH            = 1;
const uint32_t HTTP2_MAX_CONCURRENT_STREAMS = UINT_MAX;
const uint32_t HTTP2_INITIAL_WINDOW_SIZE    = 65535;
const uint32_t HTTP2_MAX_FRAME_SIZE         = 16384;
const uint32_t HTTP2_HEADER_TABLE_SIZE      = 4096;
const uint32_t HTTP2_MAX_HEADER_LIST_SIZE   = UINT_MAX;
const uint32_t HTTP2_MAX_BUFFER_USAGE       = 524288;

// [RFC 7540] 5.3.5 Default Priorities
// The RFC says weight value is 1 to 256, but the value in TS is between 0 to 255
// to use uint8_t. So the default weight is 16 minus 1.
const uint32_t HTTP2_PRIORITY_DEFAULT_STREAM_DEPENDENCY = 0;
const uint8_t  HTTP2_PRIORITY_DEFAULT_WEIGHT            = 15;

// Statistics
struct Http2StatsBlock {
  Metrics::Gauge::AtomicType   *current_client_session_count;
  Metrics::Gauge::AtomicType   *current_server_session_count;
  Metrics::Gauge::AtomicType   *current_active_client_connection_count;
  Metrics::Gauge::AtomicType   *current_active_server_connection_count;
  Metrics::Gauge::AtomicType   *current_client_stream_count;
  Metrics::Gauge::AtomicType   *current_server_stream_count;
  Metrics::Counter::AtomicType *total_client_stream_count;
  Metrics::Counter::AtomicType *total_server_stream_count;
  Metrics::Counter::AtomicType *total_transactions_time;
  Metrics::Counter::AtomicType *total_client_connection_count;
  Metrics::Counter::AtomicType *total_server_connection_count;
  Metrics::Counter::AtomicType *stream_errors_count;
  Metrics::Counter::AtomicType *connection_errors_count;
  Metrics::Counter::AtomicType *session_die_default;
  Metrics::Counter::AtomicType *session_die_other;
  Metrics::Counter::AtomicType *session_die_active;
  Metrics::Counter::AtomicType *session_die_inactive;
  Metrics::Counter::AtomicType *session_die_eos;
  Metrics::Counter::AtomicType *session_die_error;
  Metrics::Counter::AtomicType *session_die_high_error_rate;
  Metrics::Counter::AtomicType *max_settings_per_frame_exceeded;
  Metrics::Counter::AtomicType *max_settings_per_minute_exceeded;
  Metrics::Counter::AtomicType *max_settings_frames_per_minute_exceeded;
  Metrics::Counter::AtomicType *max_ping_frames_per_minute_exceeded;
  Metrics::Counter::AtomicType *max_priority_frames_per_minute_exceeded;
  Metrics::Counter::AtomicType *max_rst_stream_frames_per_minute_exceeded;
  Metrics::Counter::AtomicType *max_continuation_frames_per_minute_exceeded;
  Metrics::Counter::AtomicType *insufficient_avg_window_update;
  Metrics::Counter::AtomicType *max_concurrent_streams_exceeded_in;
  Metrics::Counter::AtomicType *max_concurrent_streams_exceeded_out;
  Metrics::Counter::AtomicType *data_frames_in;
  Metrics::Counter::AtomicType *headers_frames_in;
  Metrics::Counter::AtomicType *priority_frames_in;
  Metrics::Counter::AtomicType *rst_stream_frames_in;
  Metrics::Counter::AtomicType *settings_frames_in;
  Metrics::Counter::AtomicType *push_promise_frames_in;
  Metrics::Counter::AtomicType *ping_frames_in;
  Metrics::Counter::AtomicType *goaway_frames_in;
  Metrics::Counter::AtomicType *window_update_frames_in;
  Metrics::Counter::AtomicType *continuation_frames_in;
  Metrics::Counter::AtomicType *unknown_frames_in;
};

extern Http2StatsBlock http2_rsb;

// [RFC 7540] 6.9.1. The Flow Control Window
static const Http2WindowSize HTTP2_MAX_WINDOW_SIZE = 0x7FFFFFFF;

// [RFC 7540] 5.4. Error Handling
enum class Http2ErrorClass {
  HTTP2_ERROR_CLASS_NONE,
  HTTP2_ERROR_CLASS_CONNECTION,
  HTTP2_ERROR_CLASS_STREAM,
};

// [RFC 7540] 7. Error Codes
enum class Http2ErrorCode {
  HTTP2_ERROR_NO_ERROR            = 0,
  HTTP2_ERROR_PROTOCOL_ERROR      = 1,
  HTTP2_ERROR_INTERNAL_ERROR      = 2,
  HTTP2_ERROR_FLOW_CONTROL_ERROR  = 3,
  HTTP2_ERROR_SETTINGS_TIMEOUT    = 4,
  HTTP2_ERROR_STREAM_CLOSED       = 5,
  HTTP2_ERROR_FRAME_SIZE_ERROR    = 6,
  HTTP2_ERROR_REFUSED_STREAM      = 7,
  HTTP2_ERROR_CANCEL              = 8,
  HTTP2_ERROR_COMPRESSION_ERROR   = 9,
  HTTP2_ERROR_CONNECT_ERROR       = 10,
  HTTP2_ERROR_ENHANCE_YOUR_CALM   = 11,
  HTTP2_ERROR_INADEQUATE_SECURITY = 12,
  HTTP2_ERROR_HTTP_1_1_REQUIRED   = 13,

  HTTP2_ERROR_MAX,
};

// [RFC 7540] 5.1. Stream States
enum class Http2StreamState {
  HTTP2_STREAM_STATE_IDLE,
  HTTP2_STREAM_STATE_RESERVED_LOCAL,
  HTTP2_STREAM_STATE_RESERVED_REMOTE,
  HTTP2_STREAM_STATE_OPEN,
  HTTP2_STREAM_STATE_HALF_CLOSED_LOCAL,
  HTTP2_STREAM_STATE_HALF_CLOSED_REMOTE,
  HTTP2_STREAM_STATE_CLOSED
};

enum Http2FrameType {
  HTTP2_FRAME_TYPE_DATA          = 0,
  HTTP2_FRAME_TYPE_HEADERS       = 1,
  HTTP2_FRAME_TYPE_PRIORITY      = 2,
  HTTP2_FRAME_TYPE_RST_STREAM    = 3,
  HTTP2_FRAME_TYPE_SETTINGS      = 4,
  HTTP2_FRAME_TYPE_PUSH_PROMISE  = 5,
  HTTP2_FRAME_TYPE_PING          = 6,
  HTTP2_FRAME_TYPE_GOAWAY        = 7,
  HTTP2_FRAME_TYPE_WINDOW_UPDATE = 8,
  HTTP2_FRAME_TYPE_CONTINUATION  = 9,

  HTTP2_FRAME_TYPE_MAX,
};

extern Metrics::Counter::AtomicType *http2_frame_metrics_in[HTTP2_FRAME_TYPE_MAX + 1];

// [RFC 7540] 6.1. Data
enum Http2FrameFlagsData {
  HTTP2_FLAGS_DATA_END_STREAM = 0x01,
  HTTP2_FLAGS_DATA_PADDED     = 0x08,

  HTTP2_FLAGS_DATA_MASK = 0x09,
};

// [RFC 7540] 6.2. Headers
enum Http2FrameFlagsHeaders {
  HTTP2_FLAGS_HEADERS_END_STREAM  = 0x01,
  HTTP2_FLAGS_HEADERS_END_HEADERS = 0x04,
  HTTP2_FLAGS_HEADERS_PADDED      = 0x08,
  HTTP2_FLAGS_HEADERS_PRIORITY    = 0x20,

  HTTP2_FLAGS_HEADERS_MASK = 0x2D,
};

// [RFC 7540] 6.3. Priority
enum Http2FrameFlagsPriority {
  HTTP2_FLAGS_PRIORITY_MASK = 0x00,
};

// [RFC 7540] 6.4. Rst Stream
enum Http2FrameFlagsRstStream {
  HTTP2_FLAGS_RST_STREAM_MASK = 0x00,
};

// [RFC 7540] 6.5. Settings
enum Http2FrameFlagsSettings {
  HTTP2_FLAGS_SETTINGS_ACK = 0x01,

  HTTP2_FLAGS_SETTINGS_MASK = 0x01
};

// [RFC 7540] 6.6. Push Promise
enum Http2FrameFlagsPushPromise {
  HTTP2_FLAGS_PUSH_PROMISE_END_HEADERS = 0x04,
  HTTP2_FLAGS_PUSH_PROMISE_PADDED      = 0x08,

  HTTP2_FLAGS_PUSH_PROMISE_MASK = 0x0C,
};

// [RFC 7540] 6.7. Ping
enum Http2FrameFlagsPing {
  HTTP2_FLAGS_PING_ACK = 0x01,

  HTTP2_FLAGS_PING_MASK = 0x01
};

// [RFC 7540] 6.8. Goaway
enum Http2FrameFlagsGoaway {
  HTTP2_FLAGS_GOAWAY_MASK = 0x00,
};

// [RFC 7540] 6.9. Window Update
enum Http2FrameFlagsWindowUpdate {
  HTTP2_FLAGS_WINDOW_UPDATE_MASK = 0x00,
};

// [RFC 7540] 6.10. Continuation
enum Http2FrameFlagsContinuation {
  HTTP2_FLAGS_CONTINUATION_END_HEADERS = 0x04,

  HTTP2_FLAGS_CONTINUATION_MASK = 0x04,
};

// [RFC 7540] 6.5.2. Defined SETTINGS Parameters
enum Http2SettingsIdentifier {
  HTTP2_SETTINGS_HEADER_TABLE_SIZE      = 1,
  HTTP2_SETTINGS_ENABLE_PUSH            = 2,
  HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS = 3,
  HTTP2_SETTINGS_INITIAL_WINDOW_SIZE    = 4,
  HTTP2_SETTINGS_MAX_FRAME_SIZE         = 5,
  HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE   = 6,
  HTTP2_SETTINGS_MAX, // Really just the max of the "densely numbered" core id's
};

// [RFC 7540] 4.1. Frame Format
struct Http2FrameHeader {
  uint32_t      length;
  uint8_t       type;
  uint8_t       flags;
  Http2StreamId streamid;
};

// [RFC 7540] 5.4. Error Handling
struct Http2Error {
  Http2Error(const Http2ErrorClass error_class = Http2ErrorClass::HTTP2_ERROR_CLASS_NONE,
             const Http2ErrorCode error_code = Http2ErrorCode::HTTP2_ERROR_NO_ERROR, const char *err_msg = "")
  {
    cls  = error_class;
    code = error_code;
    msg  = err_msg;
  };

  Http2ErrorClass cls;
  Http2ErrorCode  code;
  const char     *msg;
};

// [RFC 7540] 6.5.1. SETTINGS Format
struct Http2SettingsParameter {
  uint16_t id;
  uint32_t value;
};

// [RFC 7540] 6.3 PRIORITY Format
struct Http2Priority {
  Http2Priority() : weight(HTTP2_PRIORITY_DEFAULT_WEIGHT), stream_dependency(HTTP2_PRIORITY_DEFAULT_STREAM_DEPENDENCY) {}

  bool     exclusive_flag = false;
  uint8_t  weight;
  uint32_t stream_dependency;
};

// [RFC 7540] 6.2 HEADERS Format
struct Http2HeadersParameter {
  Http2HeadersParameter() {}
  uint8_t       pad_length = 0;
  Http2Priority priority;
};

// [RFC 7540] 6.8 GOAWAY Format
struct Http2Goaway {
  Http2Goaway() {}
  Http2StreamId  last_streamid = 0;
  Http2ErrorCode error_code    = Http2ErrorCode::HTTP2_ERROR_NO_ERROR;

  // NOTE: we don't (de)serialize the variable length debug data at this layer
  // because there's
  // really nothing we can do with it without some out of band agreement. Trying
  // to deal with it
  // just complicates memory management.
};

// [RFC 7540] 6.4 RST_STREAM Format
struct Http2RstStream {
  uint32_t error_code;
};

// [RFC 7540] 6.6 PUSH_PROMISE Format
struct Http2PushPromise {
  uint8_t       pad_length        = 0;
  Http2StreamId promised_streamid = 0;
};

static inline bool
http2_is_client_streamid(Http2StreamId streamid)
{
  return (streamid & 0x1u) == 0x1u;
}

static inline bool
http2_is_server_streamid(Http2StreamId streamid)
{
  return (streamid & 0x1u) == 0x0u && streamid != 0x0u;
}

bool http2_parse_frame_header(IOVec, Http2FrameHeader &);

bool http2_write_frame_header(const Http2FrameHeader &, IOVec);

bool http2_write_rst_stream(uint32_t, IOVec);

bool http2_write_settings(const Http2SettingsParameter &, const IOVec &);

bool http2_write_ping(const uint8_t *, IOVec);

bool http2_write_goaway(const Http2Goaway &, IOVec);

bool http2_write_window_update(const uint32_t new_size, const IOVec &);

bool http2_write_push_promise(const Http2PushPromise &push_promise, const uint8_t *src, size_t length, const IOVec &iov);

bool http2_frame_header_is_valid(const Http2FrameHeader &, unsigned);

bool http2_settings_parameter_is_valid(const Http2SettingsParameter &);

bool http2_parse_headers_parameter(IOVec, Http2HeadersParameter &);

bool http2_parse_priority_parameter(IOVec, Http2Priority &);

bool http2_parse_rst_stream(IOVec, Http2RstStream &);

bool http2_parse_settings_parameter(IOVec, Http2SettingsParameter &);

bool http2_parse_goaway(IOVec, Http2Goaway &);

bool http2_parse_window_update(IOVec, uint32_t &);

Http2ErrorCode http2_decode_header_blocks(HTTPHdr *, const uint8_t *, const uint32_t, uint32_t *, HpackHandle &, bool, uint32_t,
                                          bool is_outbound = false);

Http2ErrorCode http2_encode_header_blocks(HTTPHdr *, uint8_t *, uint32_t, uint32_t *, HpackHandle &, int32_t);

ParseResult http2_convert_header_from_2_to_1_1(HTTPHdr *);
ParseResult http2_convert_header_from_1_1_to_2(HTTPHdr *);
void        http2_init();

/** Each of these values correspond to the flow control policy described in or
 * records.yaml documentation for proxy.config.http2.flow_control.policy_in.
 */
enum class Http2FlowControlPolicy {
  STATIC_SESSION_AND_STATIC_STREAM,
  LARGE_SESSION_AND_STATIC_STREAM,
  LARGE_SESSION_AND_DYNAMIC_STREAM,
};

// Not sure where else to put this, but figure this is as good of a start as
// anything else.
// Right now, only the static init() is available, which sets up some basic
// librecords
// dependencies.
class Http2
{
public:
  static uint32_t               max_concurrent_streams_in;
  static uint32_t               min_concurrent_streams_in;
  static uint32_t               max_active_streams_in;
  static bool                   throttling;
  static uint32_t               stream_priority_enabled;
  static uint32_t               initial_window_size_in;
  static Http2FlowControlPolicy flow_control_policy_in;
  static uint32_t               max_frame_size;
  static uint32_t               header_table_size;
  static uint32_t               max_header_list_size;
  static uint32_t               accept_no_activity_timeout;
  static uint32_t               no_activity_timeout_in;
  static uint32_t               active_timeout_in;
  static uint32_t               push_diary_size;
  static uint32_t               zombie_timeout_in;

  static uint32_t               max_concurrent_streams_out;
  static uint32_t               min_concurrent_streams_out;
  static uint32_t               max_active_streams_out;
  static uint32_t               no_activity_timeout_out;
  static uint32_t               initial_window_size_out;
  static Http2FlowControlPolicy flow_control_policy_out;

  static float    stream_error_rate_threshold;
  static uint32_t stream_error_sampling_threshold;
  static uint32_t max_settings_per_frame;
  static uint32_t max_settings_per_minute;
  static uint32_t max_settings_frames_per_minute;
  static uint32_t max_ping_frames_per_minute;
  static uint32_t max_priority_frames_per_minute;
  static uint32_t max_rst_stream_frames_per_minute;
  static uint32_t max_continuation_frames_per_minute;
  static float    min_avg_window_update;
  static uint32_t con_slow_log_threshold;
  static uint32_t stream_slow_log_threshold;
  static uint32_t header_table_size_limit;
  static uint32_t write_buffer_block_size;
  static float    write_size_threshold;
  static uint32_t write_time_threshold;
  static uint32_t buffer_water_mark;

  static void init();
};
