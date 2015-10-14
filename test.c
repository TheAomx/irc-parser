#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "test.h"

#define IRC_PARSER_TEST_BUFFER_SIZE 10000

#define CMP_RESULTS(v) do {\
  if (compare_strings(test->v, result->v, result->v##_len) == false) {\
    return false;\
  }\
} while(0)

static inline bool compare_strings(const char *expected, const char *got, size_t length) {
    if (expected == NULL && got == NULL)
        return true;
    
    if (expected == NULL || got == NULL)
        return false;
    
    if (strlen(expected) != length) 
        return false;
    
    return strncmp(got, expected, length) == 0;
}

static bool compare_test_results(const irc_parser_test_case *test, irc_parser_test_result *result) {
    CMP_RESULTS(nick);
    CMP_RESULTS(name);
    CMP_RESULTS(host);
    CMP_RESULTS(command);
    CMP_RESULTS(param);
    
    return true;
}


#define CPY_EXPECTED_RESULTS(k)                 \
  strncpy(k, res->k, res->k##_len);             \
  k[res->k##_len] = '\0'                     


#define NUM_TESTS (sizeof(cases) / sizeof(irc_parser_test_case))

/*
     * From RFC 1459:
     *  <message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
     *  <prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
     *  <command>  ::= <letter> { <letter> } | <number> <number> <number>
     *  <SPACE>    ::= ' ' { ' ' }
     *  <params>   ::= <SPACE> [ ':' <trailing> | <middle> <params> ]
     *  <middle>   ::= <Any *non-empty* sequence of octets not including SPACE
     *                 or NUL or CR or LF, the first of which may not be ':'>
     *  <trailing> ::= <Any, possibly *empty*, sequence of octets not including
     *                   NUL or CR or LF>
*/

const irc_parser_test_case cases[] = {
    {":user JOIN :#channel\r\n",
        "user",
        NULL,
        NULL,
        "JOIN",
        "#channel"},
    {":user!name JOIN :#channel\r\n",
        "user",
        "name",
        NULL,
        "JOIN",
        "#channel"},
    {":user@host JOIN :#channel\r\n",
        "user",
        NULL,
        "host",
        "JOIN",
        "#channel"},
    {":user!name@host JOIN :#channel\r\n",
        "user",
        "name",
        "host",
        "JOIN",
        "#channel"},
    { "PRIVMSG #test :hello world!\r\n"
        , NULL
        , NULL
        , NULL
        , "PRIVMSG"
        , "#test hello world!"},
    { ":lohkey!name@host PRIVMSG #test :hello test script!\r\n"
        , "lohkey"
        , "name"
        , "host"
        , "PRIVMSG"
        , "#test hello test script!"},
    { "PRIVMSG lohkey :boo!\r\n"
        , NULL
        , NULL
        , NULL
        , "PRIVMSG"
        , "lohkey boo!"},
    { ":lohkey!name@host NOTICE test :PM me again and ban hammer4u\r\n"
        , "lohkey"
        , "name"
        , "host"
        , "NOTICE"
        , "test PM me again and ban hammer4u"},
    { "NOTICE lohkey :what about notices?\r\n"
        , NULL
        , NULL
        , NULL
        , "NOTICE"
        , "lohkey what about notices?"},
    { "KICK #test test :for testing purposes\r\n"
        , NULL
        , NULL
        , NULL
        , "KICK"
        , "#test test for testing purposes"},
    { ":lohkey!name@host KICK #test test :for testing purposes\r\n"
        , "lohkey"
        , "name"
        , "host"
        , "KICK"
        , "#test test for testing purposes"}
};

const irc_parser_negative_test_case negative_cases[] = {
    {":user!name@host JOIN :#channel\r\n",
            IRC_ERROR_NONE},
    {":alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers_alongusernamethatwillbreakbuffers JOIN :#channel\r\n",
        IRC_ERROR_LENGTH},
};

// global state .... yup don't care right now
irc_parser_test_result result;
irc_parser parser;
irc_parser_settings settings;
char param_buffer[IRC_PARSER_TEST_BUFFER_SIZE];
int current_case = 0;
int all_passing = 1;
int tests_ran = 0;
char *input;

size_t _get_total_input_size() {
    size_t acc = 0;
    for (size_t i = 0; i < NUM_TESTS; i++) {
        acc += strlen(cases[i].test);
    }
    return acc;

}

void _build_input_buffer() {
  input = calloc(_get_total_input_size(), sizeof(char));
  for (size_t i = 0; i < NUM_TESTS; i++) {
    strcat(input, cases[i].test);
  }
}

int on_nick(irc_parser *parser, const char *at, size_t len) {
  irc_parser_test_result *res = parser->data;
  res->nick = at;
  res->nick_len = len;
  return 0;
}

int on_name(irc_parser *parser, const char *at, size_t len) {
  irc_parser_test_result *res = parser->data;
  res->name = at;
  res->name_len = len;
  return 0;
}

int on_host(irc_parser *parser, const char *at, size_t len) {
  irc_parser_test_result *res = parser->data;
  res->host = at;
  res->host_len = len;
  return 0;
}

int on_command(irc_parser *parser, const char *at, size_t len) {
  irc_parser_test_result *res = parser->data;
  res->command = at;
  res->command_len = len;
  return 0;
}

int on_param(irc_parser *parser, const char *at, size_t len) {
  irc_parser_test_result *res = parser->data;
  memcpy(&param_buffer[res->param_len], at, len);
  result.param = param_buffer;
  param_buffer[res->param_len + len++] = ' ';
  res->param_len += len;
  return 0;
}

int _passes_current_case() {
  const irc_parser_test_case *c = &cases[current_case];
  if (compare_test_results(c, &result))
    return 1;
  else
    return 0;
}

void _reset_results() {
  memset(&result, 0, sizeof(irc_parser_test_result));
}

int on_end(irc_parser *parser, const char *at, size_t len) {
  irc_parser_test_result *res = parser->data;
  param_buffer[--res->param_len] = '\0';
  int passing = _passes_current_case();
  if (passing) {
    print_test_result(passing);
    _reset_results();
    next_case();
    all_passing &= 1;
  } else {
    on_error(parser, at, len);
  }
  return 0;
}

int on_error(irc_parser *parser, const char *at, size_t len) {
  irc_parser_test_result *res = parser->data;
  print_test_result(0);
  print_expected_results(res, at, len);
  _reset_results();
  next_case();
  all_passing = 0;
  return 0;
}

void evaluate_negative_testcase(irc_parser *parser, int test_case) {
    enum irc_parser_error irc_error = irc_parser_get_error(parser);
    int passing = negative_cases[test_case].error == irc_error;
    const char *got_error = irc_parser_error_to_string(irc_error);
    const char *wanted_error = irc_parser_error_to_string(negative_cases[test_case].error);
    
    if (!passing) {
        all_passing = 0;
        printf("negative test case %d failed! got error = %s but wanted %s!", test_case, got_error, wanted_error);
    }
    
    print_test_result(passing);
}

void next_negative_case(irc_parser *parser) {
    current_case++;
    irc_parser_reset(parser);
}

int on_negative_end(irc_parser *parser, const char *at, size_t len) {
    evaluate_negative_testcase(parser, current_case);
    next_negative_case(parser);
    return 0;
}

int on_negative_error(irc_parser *parser, const char *at, size_t len) {
    evaluate_negative_testcase(parser, current_case);
    next_negative_case(parser);
    return 0;
}

void print_expected_results(irc_parser_test_result *res, const char *at, 
                            size_t len) {
  const irc_parser_test_case *c_case = &cases[current_case];
  char nick[IRC_PARSER_TEST_BUFFER_SIZE+1], 
       name[IRC_PARSER_TEST_BUFFER_SIZE+1], 
       host[IRC_PARSER_TEST_BUFFER_SIZE+1], 
       command[IRC_PARSER_TEST_BUFFER_SIZE+1], 
       param[IRC_PARSER_TEST_BUFFER_SIZE+1];
  
  printf("At 0x%p(%zu)\n", at, len);
  printf("Expected: { raw: %s"
         "          , nick: %s\n"
         "          , name: %s\n"
         "          , host: %s\n"
         "          , command: %s\n"
         "          , param: %s\n"
         "          }\n"
         , c_case->test
         , c_case->nick
         , c_case->name
         , c_case->host
         , c_case->command
         , c_case->param
         );
  CPY_EXPECTED_RESULTS(nick);
  CPY_EXPECTED_RESULTS(name);
  CPY_EXPECTED_RESULTS(host);
  CPY_EXPECTED_RESULTS(command);
  CPY_EXPECTED_RESULTS(param);
  printf("Got:      { raw: %s\n"
         "          , nick: %s\n"
         "          , name: %s\n"
         "          , host: %s\n"
         "          , command: %s\n"
         "          , param: %s\n"
         "          }\n"
         , parser.raw
         , (nick[0])    ? nick    : NULL
         , (name[0])    ? name    : NULL
         , (host[0])    ? host    : NULL
         , (command[0]) ? command : NULL
         , (param[0])   ? param   : NULL
         );
}

void print_test_result(int result) {
  printf("%c", (result) ? '.' : 'x');
  if (++tests_ran % 60 == 0) {
    printf("%c", '\n');
  }
  fflush(stdout);
}

void next_case() {
  result.param_len = 0;
  current_case = (current_case + 1) % NUM_TESTS;
  irc_parser_reset(&parser);
}

void run_sucessful_testsuite() {
    for (size_t i = 0; i < NUM_TESTS; i++) {
        const irc_parser_test_case *test_case = &cases[i];
        irc_parser_execute(&parser, test_case->test, strlen(test_case->test));
    }
}

void run_permutaded_testsuite() {
    _build_input_buffer();
    for (size_t i = 1; i < _get_total_input_size(); i++) {
        int full_peices = _get_total_input_size() / i;
        int remainding = _get_total_input_size() % i;
        for (int j = 0; j < full_peices; j++) {
            irc_parser_execute(&parser, &input[i * j], i);
        }
        if (remainding) {
            irc_parser_execute(&parser
                    , &input[_get_total_input_size() - remainding]
                    , remainding
                    );
        }
    }
}

void run_negative_testsuite() {
    size_t num_negative_tests = (sizeof(negative_cases) / sizeof(irc_parser_negative_test_case));
    
    current_case = 0;
    irc_parser negative_parser;
    irc_parser_settings negative_settings;
    irc_parser_settings_init( &negative_settings
                          , NULL
                          , NULL
                          , NULL
                          , NULL
                          , NULL
                          , on_negative_end
                          , on_negative_error
                          );
  irc_parser_init(&negative_parser, &negative_settings);
  
  for (size_t i = 0; i < num_negative_tests; i++) {
      const irc_parser_negative_test_case *test_case = &negative_cases[i];
      irc_parser_execute(&negative_parser, test_case->test, strlen(test_case->test));
  }
}

void run_tests() {
    run_sucessful_testsuite();
    run_permutaded_testsuite();
    run_negative_testsuite();
    printf("\nDone\n");

}

int main () {
  irc_parser_settings_init( &settings
                          , on_nick
                          , on_name
                          , on_host
                          , on_command
                          , on_param
                          , on_end
                          , on_error
                          );
  irc_parser_init(&parser, &settings);
  parser.data = &result;
  printf("Running test suite\n");
  printf("==================\n");
  run_tests();
  printf("Finished with %s errors\n", (all_passing) ? "no" : "some");
  return !all_passing;
}
