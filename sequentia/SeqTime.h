#pragma once

#define SEQ_TIME_BASE 1000000
#define SEQ_TIME_BASE_HALF 500000
#define SEQ_TIME_BASE_MILLISECONDS 1000

#define SEQ_TIME(seconds) (seconds) * SEQ_TIME_BASE
#define SEQ_TIME_FROM_MILLISECONDS(milliseconds) (milliseconds) * SEQ_TIME_BASE_MILLISECONDS

#define SEQ_TIME_IN_SECONDS(time) (time) / SEQ_TIME_BASE
#define SEQ_TIME_IN_MILLISECONDS(time) (time) / SEQ_TIME_BASE_MILLISECONDS

#define SEQ_TIME_TO_STREAM_TIME(time, timeBase) ((time) / (SEQ_TIME_BASE * (timeBase)))
#define STREAM_TIME_TO_SEQ_TIME(streamTime, timeBase) ((streamTime) * (SEQ_TIME_BASE * (timeBase)))

#define SEQ_TIME_FLOOR(time) (time) / SEQ_TIME_BASE * SEQ_TIME_BASE
#define SEQ_TIME_CEIL(time) ((time) + SEQ_TIME_BASE_HALF) / SEQ_TIME_BASE * SEQ_TIME_BASE
#define SEQ_TIME_ROUND(time) ((time) + SEQ_TIME_BASE_HALF + 1) / SEQ_TIME_BASE * SEQ_TIME_BASE

#define SEQ_TIME_FLOOR_IN_SECONDS(time) (time) / SEQ_TIME_BASE
#define SEQ_TIME_CEIL_IN_SECONDS(time) ((time) + SEQ_TIME_BASE_HALF) / SEQ_TIME_BASE
#define SEQ_TIME_ROUND_IN_SECONDS(time) ((time) + SEQ_TIME_BASE_HALF + 1) / SEQ_TIME_BASE
