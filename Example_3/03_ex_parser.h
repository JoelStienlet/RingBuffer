/*
 
   Copyright 2021 Joël Stienlet
 
   MIT license, see LICENSE file.
 
 */

// returned by the parser from parser_add() after insertion of some data 
enum parser_op_result {
    not_finished, // expecting more bytes
    finished_bad_pack, // finished but the packet is bad
    finished_good_pack, // finished, and the packet is good : don't forget to read the number of rejected bytes!
    error // a fatal error has been encountered: better terminate the whole process...
};

typedef struct parser_str parser_t;



size_t parser_query_skipped(parser_t *p_parser);
int parser_add_byte(parser_t *p_parser, uint8_t byte);
void print_parser_status(FILE *f, parser_t *p_parser);
int init_parser( parser_t **p_p_parser, 
                 size_t magic_size, 
                 size_t size_size, 
                 size_t chksum_size, 
                 size_t max_packet_size,
                 int (*is_magic)(void *to_check, int size_to_check));
int reset_parser(parser_t *p_parser);
int clear_parser(parser_t **p_p_parser);
