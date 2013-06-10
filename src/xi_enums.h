#ifndef __XI_ENUMS_H__
#define __XI_ENUMS_H__

// TODO: Consider Quarks in place of enums. Quarks are printable.

typedef enum {
  XI_ERROR = 1,
  XI_NULL_PARAM,
} XIErroFlags;

typedef enum {
  XI_SDL_ERROR = 1,
  XI_SDL_INIT_ERROR,
} XISDLErroFlags;

typedef enum {
  XI_DURATION_CONTINUOUS = 1,
  XI_DURATION_TRUNCATE
} XIDurationType;

/*! These values will be used in bitwise operations */
typedef enum {
  XI_UNKNOWN_EVENT = 0, /*!< Use when type not yet assigned */
  XI_SEQ_EVENT = 1,
  XI_INPUT_EVENT = 2
} XIEventType;

/*! These values MIGHT be used in bitwise operations, so plan as if they were. */
typedef enum {
  XI_EVENT_SUBTYPE_NA = 0, /*!< Use when subtype not applicable */
  XI_INPUT_XY = 1,
  XI_INPUT_Z = 2
} XIInputType;

#endif
