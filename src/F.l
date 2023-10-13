/*defintion*/
%option c++
%option yylineno

%{
    enum types {
        eof = 0,
        none,
        label,
        directive,
        comma,
        collon,
        hex_integer,
        decimal_integer,
        binary_integer,
        ch,
        reg,
        mnemonic,
        newline,
        string
    };
%}

DIGIT [0-9]
BIT [0-1]
STRING \"([^\\\"]|\\.)*\"
HEX_DIGIT [0-9A-F]|[0-9a-f]
ALPHA_NUM [A-z0-9]
LABEL [A-z]{ALPHA_NUM}* 
DIRECTIVE \.{ALPHA_NUM}+ 
INTEGER \-?{DIGIT}+
HEX_INTEGER \-?0x{HEX_DIGIT}+
BIN_INTEGER \-?0b{BIT}+
COLLON :
COMMA ,
CHAR '.'
REG (r[0-31]|zero|sp|gp|k0|k1|pg|ra|ar|s[0-7]|t[0-7]|fn[0-7])

REG_STMNT(add|sub|mult|div|multu|divu|eql|neql|grt|grtu|gre|greu|lsft|rsft|rsfta|nor|nand|or|and|xor|xnor)

IMM_STMNT(xori|ori|andi|addi|multi|divi|multui|divui|lsfti|rsfti|rsftia)

UN_STMNT(neg|not)

LD_STMNT(lw|lh|lb|lhu|lbu)

ST_STMNT(sw|sh|sb)

BR_STMNT(beq|bne|bgr|bgru|bge|bgeu)

JMP_STMNT(jal|jmp)

SET_STMNT(set)


%%

(\/\/.*) {return none;}
{STRING} {return string;}
{REG} {return reg;}
({UN_STMNT}|{LD_STMNT}|{ST_STMNT}|{BR_STMNT}|{JMP_STMNT}|{REG_STMNT}|{IMM_STMNT}|{SET_STMNT}) {return mnemonic;}
{DIRECTIVE} {return directive;}
{LABEL} {return label;}
{INTEGER} {return decimal_integer;}
{HEX_INTEGER} {return hex_integer;}
{BIN_INTEGER} {return binary_integer;}
{CHAR} {return ch;}
{COLLON} {return collon;}
{COMMA} {return comma;}
<<EOF>> {return eof;}
[\n]+ {return newline;}
[\t]+ {return none;}
. {return none;}

%%

int yyFlexLexer::yywrap() {return 0;}
