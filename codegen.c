#include "9cc.h"

static int labelseq;

static void gen_lval(Node *node) {
    if (node->kind != ND_LVAR) {
        error("not an lvalue");
    }
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
}

static void gen(Node *node) {
    switch (node->kind) {
    case ND_NUM:
        printf("  push %ld\n", node->val);
        return;
    case ND_EXPR_STMT:
        gen(node->lhs);
        printf("  add rsp, 8\n");
        return;
    case ND_LVAR:
        gen_lval(node);
        printf("  pop rax\n");
        printf("  mov rax, [rax]\n");
        printf("  push rax\n");
        return;
    case ND_ASSIGN:
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("  pop rdi\n");
        printf("  pop rax\n");
        printf("  mov [rax], rdi\n");
        printf("  push rdi\n");
        return;
    case ND_BLOCK:
        for (Node *n = node->body; n; n = n->next) {
            gen(n);
        }
        return;
    case ND_IF: {
        int seq = labelseq++;
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        if (node->els) {
            printf("  je .Lelse%d\n", seq);
            gen(node->then);
            printf("  jmp .Lend%d\n", seq);
            printf(".Lelse%d:\n", seq);
            gen(node->els);
            printf(".Lend%d:\n", seq);
        } else {
            printf("  je .Lend%d\n", seq);
            gen(node->then);
            printf(".Lend%d:\n", seq);
        }
        return;
    }
    case ND_WHILE: {
        int seq = labelseq++;
        printf(".Lbegin%d:\n", seq);
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je .Lend%d\n", seq);
        gen(node->then);
        printf("  jmp .Lbegin%d\n", seq);
        printf(".Lend%d:\n", seq);
        return;
    }
    case ND_FOR: {
        int seq = labelseq++;
        if (node->init) {
            gen(node->init);
        }
        printf(".Lbegin%d:\n", seq);
        if (node->cond) {
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je .Lend%d\n", seq);
        }
        gen(node->then);
        if (node->inc) {
            gen(node->inc);
        }
        printf("  jmp .Lbegin%d\n", seq);
        printf(".Lend%d:\n", seq);
        return;
    }
    case ND_FUNCALL:
        printf("  call %s\n", node->funcname);
        printf("  push rax\n");
        return;
    case ND_RETURN:
        gen(node->lhs);
        printf("  pop rax\n");
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch (node->kind) {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv rdi\n");
        break;
    case ND_EQ:
        printf("  cmp rax, rdi\n");
        printf("  sete al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_NE:
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LT:
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LE:
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    }

    printf("  push rax\n");
}

void codegen(Node *node) {
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    // Prologue
    // Allocating 26 variable areas.
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, 208\n");

    // Emit code
    for (int i = 0; code[i]; ++i) {
        gen(code[i]);
        printf("  pop rax\n");
    }

    // Epilogue
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}

