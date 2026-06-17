#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "interpreter.h"

int main(void)
{
    const char *source =
        "let x = 10\n"
        "const MAX = 100\n"
        "let name = \"Jayesh\"\n"
        "\n"
        "func add(a, b) -> int:\n"
        "    return a + b\n"
        "end\n"
        "\n"
        "show(\"Hello, {name}!\")\n"
        "show(add(x, 5))\n"
        "\n"
        "if x > 5:\n"
        "    show(\"High\")\n"
        "elif x > 2:\n"
        "    show(\"Mid\")\n"
        "else:\n"
        "    show(\"Low\")\n"
        "end\n"
        "\n"
        "loop i from 1 to 5:\n"
        "    show(i)\n"
        "end\n"
        "\n"
        "let nums = [10, 20, 30, 40]\n"
        "show(nums[0])\n"
        "\n"
        "match x:\n"
        "    10 -> show(\"ten\")\n"
        "    _  -> show(\"other\")\n"
        "end\n";

    Lexer *lexer = lexer_init(source);
    Parser *parser = parser_init(lexer);
    
    ASTNode *ast = parse(parser);

    Interpreter* interp = interpreter_init();
    VelValue* result = eval(ast, interp->global, NULL);
    free_value(result);

    interpreter_free(interp);
    free_ast(ast);
    free(parser);
    free(lexer);

    return EXIT_SUCCESS;
}
