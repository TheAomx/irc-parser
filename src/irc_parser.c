#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "irc_parser.h"

#define IRC_PARSER_CALL_AND_PROGRESS_ON(_parser, _a, _b) do { \
  if (_a == _b) { _irc_parser_call_and_progress(parser); }    \
} while(0)

#define IRC_PARSER_APPEND_RAW(_parser, _a) do {           \
  _parser->raw[_parser->len++] = _a;                          \
  if (_parser->len > 512) {                                   \
    _parser->state = IRC_STATE_ERROR;                         \
    _parser->error = IRC_ERROR_LENGTH;                        \
  }                                                           \
} while(0)

//// private

irc_parser_cb _irc_parser_get_cb(irc_parser *parser) {
  switch(parser->state) {
  case IRC_STATE_INIT:
  case IRC_STATE_NICK:     return parser->on_nick;
  case IRC_STATE_NAME:     return parser->on_name;
  case IRC_STATE_HOST:     return parser->on_host;
  case IRC_STATE_COMMAND:  return parser->on_command;
  case IRC_STATE_PARAMS:   return parser->on_param;
  case IRC_STATE_TRAILING: return parser->on_param;
  case IRC_STATE_END:      return parser->on_param;
  case IRC_STATE_ERROR:    return parser->on_error;
  default:                 return NULL;
  }
}

enum irc_parser_state _irc_get_next_state(irc_parser *parser) {
  switch(parser->state) {
  case IRC_STATE_INIT:     return IRC_STATE_NICK;
  case IRC_STATE_NICK:     return IRC_STATE_NAME;
  case IRC_STATE_NAME:     return IRC_STATE_HOST;
  case IRC_STATE_HOST:     return IRC_STATE_COMMAND;
  case IRC_STATE_COMMAND:  return IRC_STATE_PARAMS;
  case IRC_STATE_PARAMS:   return IRC_STATE_TRAILING;
  case IRC_STATE_TRAILING: return IRC_STATE_END;
  case IRC_STATE_END:      return IRC_STATE_INIT;
  case IRC_STATE_ERROR:    return IRC_STATE_ERROR;
  default:                 return IRC_STATE_ERROR;;
  }
}

void _irc_parser_call(irc_parser *parser) {
  irc_parser_cb f = _irc_parser_get_cb(parser);

  if (f == NULL) { return; }

  int result = f( parser
                , &parser->raw[parser->last]
                , parser->len - parser->last - 1
                );

  // TODO: error checking
  if (result) {
    //
  }
}

void _irc_parser_progress_state(irc_parser *parser) {
  parser->state = _irc_get_next_state(parser);
  parser->last = parser->len;
}

void _irc_parser_call_and_progress(irc_parser *parser) {
  _irc_parser_call(parser);
  _irc_parser_progress_state(parser);
}

void _irc_parser_trigger_error(irc_parser *parser, const char *data, int offset,
                               size_t len, enum irc_parser_error error) {
  parser->error = error;
  parser->state = IRC_STATE_ERROR;
  if (parser->on_error != NULL) {
    parser->on_error(parser, &data[offset], len - offset);
  }
}

//// public

void irc_parser_init(irc_parser *parser, irc_parser_settings *settings) {
  memcpy(parser, settings, sizeof(irc_parser_settings));
  irc_parser_reset(parser);
}

void irc_parser_reset(irc_parser *parser) {
  parser->len    = 0;
  parser->last   = 0;
  parser->state  = IRC_STATE_INIT;
  parser->raw[0] = '\0';  
}

size_t irc_parser_execute(irc_parser *parser, const char *data, size_t len) {
  for (int i = 0; i < len; i++) {
    switch(data[i]){
    case '\r':
      parser->state = IRC_STATE_END;
      break;
    case '\n':
      if (parser->state == IRC_STATE_END) {
        _irc_parser_call(parser);
        irc_parser_reset(parser);
      } else {
        return -1;
      }
      break;
    default:
      IRC_PARSER_APPEND_RAW(parser, data[i]);
      switch(parser->state) {
      case IRC_STATE_INIT:
        if (data[i] == ':') {
          parser->last = 1;
          parser->state = IRC_STATE_NICK;
        } else {
          i--;
          parser->state = IRC_STATE_COMMAND;
        }
        break;
      case IRC_STATE_NICK:
        IRC_PARSER_CALL_AND_PROGRESS_ON(parser, data[i], '!');
        break;
      case IRC_STATE_NAME:
        IRC_PARSER_CALL_AND_PROGRESS_ON(parser, data[i], '@');
        break;
      case IRC_STATE_HOST:
        IRC_PARSER_CALL_AND_PROGRESS_ON(parser, data[i], ' ');
        break;
      case IRC_STATE_COMMAND:
        IRC_PARSER_CALL_AND_PROGRESS_ON(parser, data[i], ' ');
        break;
      case IRC_STATE_PARAMS:
        if (data[i] == ' ') {
          _irc_parser_call(parser);
          parser->last = parser->len;
        } else if (data[i] == ':'  && parser->len == (parser->last + 1)) {
          _irc_parser_progress_state(parser);
        }
        break;
      default:
        parser->error = IRC_ERROR_UNDEF_STATE;
      case IRC_STATE_ERROR:
        // pass remaining data to the error callback
        _irc_parser_trigger_error(parser, data, i, len, parser->error);
        return i;
      }
    }
  }
  return len;
}

enum irc_parser_error irc_parser_get_error(irc_parser *parser) {
  return IRC_ERROR_USER;
}

const char*  irc_parser_error_string(irc_parser *parser) {
  return "";
}

void irc_parser_settings_init(irc_parser_settings *settings,
                              irc_parser_cb on_nick,
                              irc_parser_cb on_name,
                              irc_parser_cb on_host,
                              irc_parser_cb on_command,
                              irc_parser_cb on_param,
                              irc_parser_cb on_end,
                              irc_parser_cb on_error) {
  settings->on_nick    = on_nick;
  settings->on_name    = on_name;
  settings->on_host    = on_host;
  settings->on_command = on_command;
  settings->on_param   = on_param;
  settings->on_end     = on_end;
  settings->on_error   = on_error;
}

