#include "9cc.h"

char *user_input;
Token *token;

// Reports an error and exit.
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Reports an error location and exit.
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, ""); // print pos spaces.
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// Consumes the current token if it matches `op`.
bool consume(char *op) {
    if (token->kind != TK_RESERVED ||
            strlen(op) != token->len ||
            strncmp(token->str, op, token->len)) {
        return false;
    }
    token = token->next;
    return true;
}

// Consumes the current token if it is an identifier.
Token *consume_ident(void) {
    if (token->kind != TK_IDENT) {
        return NULL;
    }
    Token *t = token;
    token = token->next;
    return t;
}

// Ensure that the current token is `op`.
void expect(char *op) {
    if (token->kind != TK_RESERVED ||
            strlen(op) != token->len ||
            strncmp(token->str, op, token->len)) {
        error_at(token->str, "expected \"%s\"", op);
    }
    token = token->next;
}

// Ensure that the current token is TK_NUM.
long expect_number(void) {
    if (token->kind != TK_NUM) {
        error_at(token->str, "expected a number");
    }
    long val = token->val;
    token = token->next;
    return val;
}

bool at_eof(void) {
    return token->kind == TK_EOF;
}

// Create a new token and add it as the next token of `cur`.
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind  = kind;
    tok->str   = str;
    tok->len   = len;
    cur->next  = tok;
    return tok;
}

static bool startswith(char *p, char *q) {
    return strncmp(p, q, strlen(q)) == 0;
}

static bool is_alpha(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_alnum(char c) {
    return is_alpha(c) || ('0' <= c && c <= '9');
}

static int is_reserved(char *c) {
    static char *rw[] = { // Reserved words
        "return",
        "if",
        "else",
        "while",
        "for",
    };

    for (int i = 0; i < sizeof(rw) / sizeof(*rw); ++i) {
        int len = strlen(rw[i]);
        if (startswith(c, rw[i]) && !is_alnum(len)) {
            return len;
        }
    }
    return 0;
}

// Tokenize `user_input` and returns new tokens.
Token *tokenize(void) {
    char *p = user_input;
    Token head = {};
    Token *cur = &head;

    while (*p) {
        // Skip whitespace characters.
        if (isspace(*p)) {
            ++p;
            continue;
        }

        // Keywords
        if (is_reserved(p)) {
            int len = is_reserved(p);
            cur = new_token(TK_RESERVED, cur, p, len);
            p += len;
            continue;
        }

        // Identifier
        if (is_alpha(*p)) {
            char *q = p++;
            while (is_alnum(*p)) {
                ++p;
            }
            cur = new_token(TK_IDENT, cur, q, p - q);
            continue;
        }

        // Multi-letter punctuators
        if (startswith(p, "==") || startswith(p, "!=") ||
                startswith(p, "<=") || startswith(p, ">=")) {
            cur = new_token(TK_RESERVED, cur, p, 2);
            p += 2;
            continue;
        }

        // Single-letter punctuators
        if (ispunct(*p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        // Integer literal
        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        error_at(p, "invalid token");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

